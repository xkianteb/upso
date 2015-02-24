import random,sys,os,time
from sets import Set
from multiprocessing import Process,Queue

XMIN = 0
XMAX = 100
YMIN = 0
YMAX = 10
SPEED_SCALAR = 1
NUM_THREADS = int(sys.argv[sys.argv.index("-t")+1]) if "-t" in sys.argv else 1
TERMINATOR = "TERMINATOR"
POINT = "POINT"
CMD = "CMD"

num_points = int(sys.argv[sys.argv.index("-p")+1]) if "-p" in sys.argv else 100
steps = int(sys.argv[sys.argv.index("-s")+1]) if "-s" in sys.argv else 500

random.seed()

class Point:
	def __init__(self, coords):
		self.x = coords[0]
		self.y = coords[1]
		self.xdir = SPEED_SCALAR * (0.5 - random.random())
		self.ydir = SPEED_SCALAR * (0.5 - random.random())
	
	def to_string(self):
		return " ".join([str(round(x,3)) for x in [self.x,self.y]])
	
	def move(self):
		if self.x + self.xdir < XMIN:
			self.x = XMIN + (abs(self.xdir) - (self.x - XMIN))
			self.xdir *= -1
		elif self.x + self.xdir > XMAX:
			self.x = XMAX - self.xdir - (XMAX - self.x)
			self.xdir *= -1
		else:
			self.x += self.xdir
		
		if self.y + self.ydir < YMIN:
			self.y = YMIN + (abs(self.ydir) - (self.y - YMIN))
			self.ydir *= -1
		elif self.y + self.ydir > YMAX:
			self.y = YMAX - self.ydir - (YMAX - self.y)
			self.ydir *= -1
		else:
			self.y += self.ydir

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


