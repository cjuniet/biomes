// This is my (Christophe Juniet) modified version of
// http://staffwww.itn.liu.se/~stegu/aqsis/aqsis-newnoise/simplexnoise1234.h
//
// SimplexNoise1234
// Copyright (c) 2003-2011, Stefan Gustavson
//
// Contact: stegu@itn.liu.se
//
// This library is public domain software, released by the author
// into the public domain in February 2011. You may do anything
// you like with it. You may even remove all attributions,
// but of course I'd appreciate it if you kept my name somewhere.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.

namespace simplex {
float noise(float x);
float noise(float x, float y);
float noise(float x, float y, float z);
float noise(float x, float y, float z, float w);

float fBm1D(float x, float scale = 1.0f, int octaves = 8,
            float lacunarity = 2.0f, float persistence = 0.5f);
float fBm2D(float x, float y, float scale = 1.0f, int octaves = 8,
            float lacunarity = 2.0f, float persistence = 0.5f);
float fBm3D(float x, float y, float z, float scale = 1.0f, int octaves = 8,
            float lacunarity = 2.0f, float persistence = 0.5f);
float fBm4D(float x, float y, float z, float w, float scale = 1.0f,
            int octaves = 8, float lacunarity = 2.0f, float persistence = 0.5f);
}
