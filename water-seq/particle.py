import matplotlib.pyplot as plt
import numpy as np

epsilon = 1
sigma=1

class Particle():
  def __init__(self, x0, x1):
    self.x = np.array([x0,x1],dtype=float)
    theta = np.random.random() * np.pi * 2
    m = 0.01
    self.v = np.array([np.cos(theta)*m,np.sin(theta)*m])
    self.a = np.zeros(2)
  def updateA(self, particles):
    f = np.zeros(2)
    for p in particles:
      if p != self:
        r = np.linalg.norm(self.x - p.x)
        m = epsilon * (np.power(sigma/r,12) - np.power(sigma/r,6))
        f += (self.x - p.x) * m 
    self.a = f * .001
  def updateX(self):
    self.v+=self.a
    self.x+=self.v

def threshold(x,y,ps):
  a = np.array([x,y])
  for p in ps:
    l = np.linalg.norm(a-p.x)
    if l < 0.8:
      return True
  return False

def march(x,y,w,ps,ax):
  a = threshold(x,y,ps)
  b = threshold(x+w,y,ps)
  c = threshold(x+w,y+w,ps)
  d = threshold(x,y+w,ps)
  v1 = [x,y]
  v2 = [x+w/2,y]
  v3 = [x+w,y]
  v4 = [x+w,y+w/2]
  v5 = [x+w,y+w]
  v6 = [x+w/2,y+w]
  v7 = [x,y+w]
  v8 = [x,y+w/2]
  t = []
  if a and b and c and d:
    return
    t = [v1,v3,v5,v7]
  elif not (a or b or c or d):
    return
  elif a and not (b or c or d):
    t = [v1,v2,v8]
  elif b and not (a or c or d):
    t = [v2,v3,v4]
  elif c and not (a or b or d):
    t = [v4,v5,v6]
  elif d and not (a or b or c):
    t = [v6,v7,v8]
  elif a and b and not (c or d):
    t = [v1,v3,v4,v8]
  elif b and c and not (a or d):
    t = [v2,v3,v5,v6]
  elif c and d and not (a or b):
    t = [v8,v4,v5,v7]
  elif d and a and not (b or c):
    t = [v1,v2,v6,v7]
  elif a and c and not (b or d):
    t = [v1,v2,v4,v5,v6,v8]
  elif b and d and not (a or c):
    t = [v8,v2,v3,v4,v6,v7]
  elif a and b and c and not d:
    t = [v1,v3,v5,v6,v8]
  elif a and b and d and not c:
    t = [v1,v3,v4,v6,v7]
  elif a and c and d and not b:
    t = [v1,v2,v4,v5,v7]
  elif b and c and d and not a:
    t = [v2,v3,v5,v7,v8]
  t = np.array(t)
  ax.fill(t[:,0], t[:,1], "b")

fig, ax = plt.subplots()
ax.set(xlim=(-5, 15), ylim=(-5, 15))
particles = []
for x in range(10):
  for y in range(10):
    particles.append(Particle(x,y))
particles.append(Particle(-3,-3))

while 1:
  plt.cla()
  ax.set(xlim=(-5, 15), ylim=(-5, 15))

  for i in range(10):
    for p in particles:
      p.updateA(particles)
    for p in particles:
      p.updateX()
  for x in np.arange(-5,15,.5):
    for y in np.arange(-5,15,.5):
      march(x,y,.5,particles,ax)
  for p in particles:
    ax.plot([p.x[0]], [p.x[1]], 'ro')
  plt.pause(0.001)
