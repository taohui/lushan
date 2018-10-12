#!/usr/bin/python
import sys

for line in sys.stdin:
	tokens = line.strip().split('\t')
	print ' '.join(tokens[1:])
