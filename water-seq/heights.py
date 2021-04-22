import random
import numpy as np
import matplotlib.pyplot as plt

N = 200
b = [50*np.sin(x/20)+100 for x in range(200)]
d = [0 for x in range(200)]
for i in range(96, len(d)):
  d[i]  = i - (50*np.sin(i/20)+100)
for i in range(0, len(d)):
  d[i]  = random.randint(0,30)
s = [0 for i in range(400)]
k = 0.95
p1 = 0.5
p2 = 0

width = 1

fig, ax = plt.subplots()

ax.bar(np.arange(0, N), b, 1)
ax.bar(np.arange(0, N), d, 1, bottom=b)

def indexToIndices(i):
  return (i//2, i//2 + (i%2)*2-1)

def indicesToIndex(a1, a2):
  return a1*2 + (a2-a1+1)//2


while 1:
  outflow = [0 for i in range(400)]
  s2 = np.copy(s)
  d2 = np.copy(d)
  for i in range(1,399):
    a1, a2 = indexToIndices(i)
    outflow[i] = max((b[a1]+d[a1])-(b[a2]+d[a2]), 0)
  for i in range(400):
    s[i] = k * s2[i] + p1 * outflow[i]
  for a1 in range(200):
    if d2[a1] < s[a1*2]+s[a1*2+1]:
      l = s[a1*2]+s[a1*2+1]
      s[a1*2] = d2[a1] * s[a1*2] / l
      s[a1*2+1] = d2[a1] * s[a1*2+1] / l
    if s[a1*2]+s[a1*2+1] < p2:
      s[a1*2]=0
      s[a1*2+1]=0
  for i in range(200):
    l = -s[i*2]-s[i*2+1]
    if i>0:
      l+=s[indicesToIndex(i-1,i)]
    if i<199:
      l+=s[indicesToIndex(i+1,i)]
    d[i] = d2[i]+l

  plt.cla()
  ax.bar(np.arange(0, N), b, 1)
  ax.bar(np.arange(0, N), d, 1, bottom=b)
  plt.pause(0.0001)
