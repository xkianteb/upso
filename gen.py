import random,sys,os,time,math
from sets import Set
from multiprocessing import Process,Queue,Array

WIDTH = 10
HEIGHT = 10
SPEED_CAP = 0.2
NUM_THREADS = int(sys.argv[sys.argv.index("-t")+1]) if "-t" in sys.argv else 1
TERMINATOR = "TERMINATOR"
POINT = "POINT"
CMD = "CMD"
RANDOM = "RANDOM"
RIGHTWARD = "RIGHTWARD"

DIRECTION = RIGHTWARD if "--rightward" in sys.argv else RANDOM
PER_POINT_COORD = 6
ELASTICITY = 0.75

num_points = int(sys.argv[sys.argv.index("-p")+1]) if "-p" in sys.argv else 100
steps = int(sys.argv[sys.argv.index("-s")+1]) if "-s" in sys.argv else 500
GRID = "--grid" in sys.argv

random.seed()

def usage():
	print """
		python gen.py options
		options:
			--help : this text
			-t <num threads>
			--rightward : initialize all points with (1,0) velocity, else random
			-p <num particles> : default 100
			-s <num steps to simulate> : default 500
			--grid : align particles to x,y points
	"""

if "--help" in sys.argv:
	usage()
	sys.exit()

def vector_length(values):
	return math.sqrt(values[0]**2 + values[1]**2)

def normalize(values):
	length = vector_length(values)
	return [x / length for x in values]

def distance(x1,y1,x2,y2):
	return math.sqrt( (x2 - x1)**2 + (y2 - y1)**2 )

def dotproduct(v1, v2):
	return sum((a*b) for a, b in zip(v1, v2))

def angle(v1, v2):
	return math.acos(dotproduct(v1, v2) / (vector_length(v1) * vector_length(v2)))

def vector_angle(x,y):
	return math.tan(y / x)

class Point:
	def __init__(self, coords):
		self.x = self.format(coords[0])
		self.y = self.format(coords[1])
		self.speed = SPEED_CAP if DIRECTION == RIGHTWARD else (self.format(SPEED_CAP * random.choice([1,-1]) * random.random()))
		self.size = 0.2
		self.mass = 1
		self.angle = math.pi/2 if DIRECTION == RIGHTWARD else random.random() * 2 * math.pi
	
	def format(self, x):
		return round(x,3)
	
	def to_string(self):
		return " ".join([str(round(x,3)) for x in [self.x,self.y]])
	
	def is_colliding(self, point_coords):
		i = 0
		collides_with = []
		while i < len(point_coords):
			x,y,speed,size,mass,angle = point_coords[i:i+PER_POINT_COORD]
			i += PER_POINT_COORD
			
			if (x,y,speed,size,mass,angle) == (self.x,self.y,self.speed,self.size,self.mass,self.angle):
				continue
			
			if distance(x,y, self.x,self.y) < self.size + size:
				#print "collision between ",(x,y),(self.x,self.y)
				collides_with.append((x,y,speed,size,mass,angle))

		return collides_with
	
	def move(self, point_coords):
		collides_with = [] #self.is_colliding(point_coords)
		
		if len(collides_with) > 0:
			# do collision math
			
			x,y,speed,size,mass,angle = collides_with[0]
			
			dx = self.x - x
			dy = self.y - y
			elasticity = 0.75
			
			tangent = math.atan(dy,dx)
			ang = 0.5 * math.pi + tangent
			
			ang1 = 2*tangent - vector_angle(self.y,self.x)
			ang2 = 2*tangent - vector_angle(y,x)
			speed1 = vector_length((xvel,yvel)) * elasticity
			speed2 = vector_length((self.xvel,self.yvel)) * elasticity
			
			
			
		#(self.angle, self.speed) = addVectors((self.angle, self.speed), gravity)
		self.x += math.sin(self.angle) * self.speed
		self.y -= math.cos(self.angle) * self.speed
		
		if self.x > WIDTH - self.size:
			self.x = 2*(WIDTH - self.size) - self.x
			self.angle = - self.angle
			self.speed *= ELASTICITY
		elif self.x < self.size:
			self.x = 2*self.size - self.x
			self.angle = - self.angle
			self.speed *= ELASTICITY
			

		if self.y > HEIGHT - self.size:
			self.y = 2*(HEIGHT - self.size) - self.y
			self.angle = math.pi - self.angle
			self.speed *= ELASTICITY

		elif self.y < self.size:
			self.y = 2*self.size - self.y
			self.angle = math.pi - self.angle
			self.speed *= ELASTICITY


		self.x = self.format(self.x)
		self.y = self.format(self.y)

def threaded_work(id,input,output,output_text,point_coords):
	x = input.get()
	while not x == TERMINATOR:
		tag,p = x
		if POINT == tag:
			p.move(point_coords)
			output.put(p)
			output_text.put(p.to_string()+"\n")
			#print point_coords[0],point_coords[1]
		elif CMD == tag:
			os.system(p)
		x = input.get()

def print_locations(f,points):
	for p in points:
		f.write(p.to_string()+"\n")


stem = "step_"
outfile = "out.gif"

start = time.time()

points = []

point_coords = Array('d', range(num_points*PER_POINT_COORD) )
input_data = Queue()
for i in xrange(num_points):
	coords = (1 + i % (WIDTH-2) ,1 + (i / (WIDTH-2)) % (HEIGHT-2) ) if GRID else (random.random() * WIDTH, random.random() * HEIGHT)
	print coords
	p = Point(coords)
	points.append(p)
	point_coords[PER_POINT_COORD * i] = p.x
	point_coords[PER_POINT_COORD * i + 1] = p.y
	point_coords[PER_POINT_COORD * i + 2] = p.speed
	point_coords[PER_POINT_COORD * i + 3] = p.size
	point_coords[PER_POINT_COORD * i + 4] = p.mass
	point_coords[PER_POINT_COORD * i + 5] = p.angle
	input_data.put((POINT,p))

pad = (len(str(steps)) - len(str(0))) * "0"
filename = stem+pad+str(0)
f = open(filename+".txt","w")
print_locations(f, points)
f.close()

cmds = Queue()

output_text = Queue()
output_points = Queue()


threads = []
for t in range(NUM_THREADS):
	thr = Process(target = threaded_work, args = (t,input_data,output_points,output_text,point_coords))
	thr.start()
	threads.append(thr)

done = []
for s in xrange(steps):
	if s > 0 and s % (0.1 * steps) == 0:
		print 100.0 * s/steps,"% done",round(time.time() - start,2)
	s += 1

	cmd = "./gnu.sh "+filename+" "+(" ".join([str(x) for x in [-2,HEIGHT+2,-2,WIDTH+2]]))
	cmds.put(cmd)
	
	for p in done:
		input_data.put((POINT,p))
	
	done = []
	temp_point_coords = [0]*(num_points*PER_POINT_COORD)
	i = 0
	while not len(done) == len(points):
		p = output_points.get()
		temp_point_coords[i] = p.x
		temp_point_coords[i + 1] = p.y
		temp_point_coords[i + 2] = p.speed
		temp_point_coords[i + 3] = p.size
		temp_point_coords[i + 4] = p.mass
		temp_point_coords[i + 5] = p.angle
		i += PER_POINT_COORD
		done.append(p)

	for i,x in enumerate(temp_point_coords):
		point_coords[i] = x
	
	pad = (len(str(steps)) - len(str(s))) * "0"
	filename = stem+pad+str(s)
	f = open(filename+".txt","w")
	while not output_text.empty():
		line = output_text.get()
		f.write(line)
	f.close()


print "Time taken before gif:",time.time() - start

while not cmds.empty():
	input_data.put((CMD,cmds.get()))

for thr in threads:
	input_data.put(TERMINATOR)
	
while not input_data.empty():
	time.sleep(0.001)

if "-a" in sys.argv:
	os.system("animate "+stem+"*.png")
else:
	os.system("convert -delay 10 -loop 0 "+stem+"*.png "+outfile)
	print "Wrote out as "+outfile

os.system("rm "+stem+"*.txt")
os.system("rm "+stem+"*.png")


