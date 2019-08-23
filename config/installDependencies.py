#!/usr/bin/env python

##########################################################################
#
#  Copyright (c) 2017, Image Engine Design Inc. All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are
#  met:
#
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#
#     * Neither the name of Image Engine Design nor the names of any
#       other contributors to this software may be used to endorse or
#       promote products derived from this software without specific prior
#       written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
#  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
#  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
#  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
#  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
#  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
#  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
#  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
#  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
#  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
#  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
##########################################################################

import os
import sys
import urllib

atomsVersion = sys.argv[1]
gafferVersion = sys.argv[2]
platform = "linux"

gafferURL = "https://github.com/GafferHQ/gaffer/releases/download/{gafferVersion}/gaffer-{gafferVersion}-{platform}.tar.gz".format(
	gafferVersion = gafferVersion,
	platform = platform,
)

## \todo: get a valid Atoms download URL
atomsURL = "https://github.com/GafferHQ/gaffer/releases/download/{gafferVersion}/gaffer-{gafferVersion}-{platform}.tar.gz".format(
	gafferVersion = gafferVersion,
	platform = platform,
)

buildDir = "./artifacts"

# Get Gaffer and unpack it into the build directory

sys.stderr.write( "Downloading Gaffer \"%s\"" % gafferURL )
tarFileName, headers = urllib.urlretrieve( gafferURL )

os.makedirs( buildDir )
os.system( "tar xf %s -C %s --strip-components=1" % ( tarFileName, "buildDir/gaffer" ) )

# Get Atoms and unpack it into another directory

sys.stderr.write( "Downloading Atoms \"%s\"" % atomsURL )
tarFileName, headers = urllib.urlretrieve( atomsURL )

os.makedirs( buildDir )
os.system( "tar xf %s -C %s --strip-components=1" % ( tarFileName, "buildDir/atoms" ) )
