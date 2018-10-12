#!/usr/bin/python
import sys
import random

for line in sys.stdin:
	tokens = line.strip().split(' ')
	print "%d\t%s"%(random.randint(0, 1000000000), '\t'.join(tokens))
