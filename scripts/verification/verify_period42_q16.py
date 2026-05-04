import math
S=1<<16
a=round((math.sqrt(3)/2)*S)
b=round((math.pi*math.sin(math.radians(279)))*S)


def step(x):
    y=((x*a)>>16)-b
    return y & 0xFFFF

ok=True
for x0 in range(65536):
    x=x0
    for _ in range(42):
        x=step(x)
    if x!=x0:
        ok=False
        break
print("period42_fixed_q16:",ok)
