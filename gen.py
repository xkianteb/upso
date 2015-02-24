import random,sys,os
from sets import Set
import subprocess

XMIN = 0
XMAX = 100
YMIN = 0
YMAX = 10
SPEED_SCALAR = 1

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


def print_locations(f,points):
	for p in points:
		f.write(p.to_string()+"\n")


num_points = 100

steps = 1000
stem = "step_"
outfile = "out.gif"

points = []
for i in xrange(num_points):
	coords = (random.random() * XMAX, random.random() * YMAX)
	points.append(Point(coords))

for s in xrange(steps):
	if s > 0 and s % (0.1 * steps) == 0:
		print 100.0 * s/steps,"% done"
	s += 1
	pad = (len(str(steps)) - len(str(s))) * "0"
	filename = stem+pad+str(s)
	f = open(filename+".txt","w")
	print_locations(f, points)
	#print points[0].to_string()
	f.close()
	cmd = "./gnu.sh "+filename+" "+(" ".join([str(x) for x in [YMIN-2,YMAX+2,XMIN-2,XMAX+2]]))
	#print cmd
	os.system(cmd)
	os.system("rm "+filename+".txt")
	for p in points:
		p.move()

if "-a" in sys.argv:
	os.system("animate "+stem+"*.png")
else:
	os.system("convert -delay 10 -loop 0 "+stem+"*.png "+outfile)
	print "Wrote out as "+outfile
os.system("rm "+stem+"*.png")


