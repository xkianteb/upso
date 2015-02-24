import random,sys,os,time,math
from sets import Set
from multiprocessing import Process,Queue

XMIN = 0
XMAX = 10
YMIN = 0
YMAX = 10
SPEED_SCALAR = 0.2
NUM_THREADS = int(sys.argv[sys.argv.index("-t")+1]) if "-t" in sys.argv else 1
TERMINATOR = "TERMINATOR"
POINT = "POINT"
CMD = "CMD"
RANDOM = "RANDOM"
RIGHTWARD = "RIGHTWARD"
DIRECTION = RIGHTWARD if "--rightward" in sys.argv else RANDOM
MAX_VELOCITY = 1.0

num_points = int(sys.argv[sys.argv.index("-p")+1]) if "-p" in sys.argv else 100
steps = int(sys.argv[sys.argv.index("-s")+1]) if "-s" in sys.argv else 500

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
	"""

if "--help" in sys.argv:
	usage()
	sys.exit()

def vector_length(values):
	return math.sqrt(values[0]**2 + values[1]**2)

def normalize(values):
	length = vector_length(values)
	return [x / length for x in values]

class Point:
	def __init__(self, coords):
		self.x = coords[0]
		self.y = coords[1]
		self.xvel,self.yvel = [SPEED_SCALAR * random.choice([1,-1]) * val for val in normalize(self.velocities())]
		print self.xvel,self.yvel,vector_length([self.xvel,self.yvel])
		self.radius = 0.5
	
	def velocities(self):
		if DIRECTION == RANDOM:
			x = random.random()
			return (x,random.random())
		elif DIRECTION == RIGHTWARD:
			return (1,0)
		else:
			raise "Invalid velocity"
	
	def to_string(self):
		return " ".join([str(round(x,3)) for x in [self.x,self.y]])
	
	def move(self):
		if self.x + self.xvel < XMIN:
			self.x = XMIN + (abs(self.xvel) - (self.x - XMIN))
			self.xvel *= -1
		elif self.x + self.xvel > XMAX:
			self.x = XMAX - self.xvel - (XMAX - self.x)
			self.xvel *= -1
		else:
			self.x += self.xvel
		
		if self.y + self.yvel < YMIN:
			self.y = YMIN + (abs(self.yvel) - (self.y - YMIN))
			self.yvel *= -1
		elif self.y + self.yvel > YMAX:
			self.y = YMAX - self.yvel - (YMAX - self.y)
			self.yvel *= -1
		else:
			self.y += self.yvel

def threaded_work(id,input,output,output_text):
	x = input.get()
	while not x == TERMINATOR:
		tag,p = x
		if POINT == tag:
			p.move()
			output.put(p)
			output_text.put(p.to_string()+"\n")
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
input_data = Queue()
for i in xrange(num_points):
	coords = (random.random() * XMAX, random.random() * YMAX)
	p = Point(coords)
	points.append(p)
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
	thr = Process(target = threaded_work, args = (t,input_data,output_points,output_text))
	thr.start()
	threads.append(thr)

done = []
for s in xrange(steps):
	if s > 0 and s % (0.1 * steps) == 0:
		print 100.0 * s/steps,"% done",round(time.time() - start,2)
	s += 1

	cmd = "./gnu.sh "+filename+" "+(" ".join([str(x) for x in [YMIN-2,YMAX+2,XMIN-2,XMAX+2]]))
	cmds.put(cmd)
	
	for p in done:
		input_data.put((POINT,p))
	
	done = []
	while not len(done) == len(points):
		done.append(output_points.get())
	
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


