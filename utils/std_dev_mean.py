#Copyright 2009,2010 Alex Graves
#
# This file is part of RNNLIB.
# 
# RNNLIB is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# RNNLIB is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with RNNLIB.  If not, see <http://www.gnu.org/licenses/>.

#!/usr/bin/env python

import numpy
from math import sqrt
import sys

vals = []
for l in sys.stdin.readlines():
	vals.append(float(l))
numVals = len(vals)
stdDev = numpy.std(vals)
print numVals,"values"
print "mean",numpy.mean(vals)
print "std deviation",stdDev
print "std error",stdDev/sqrt(numVals)

