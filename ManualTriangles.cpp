#include "mat.h"
#include "wavefront.h"
#include "texture.h"

#include "orbiter.h"
#include "draw.h"
#include "app.h"

#include "vec.h"
#include "color.h"
#include "image.h"
#include "image_io.h"

#include <vector>

using namespace std;

struct Triangle
{
	vec3 pA, pB, pC;
	Color cA, cB, cC;
};

int main(int argc, char **argv)
{
	Image image(512, 512);

	vector<Triangle> tris;
	Triangle t1;
	t1.pA = vec3(-1.5f, -0.5f, 0.25f);
	t1.pB = vec3(0.5f, -0.5f, 0.25f);
	t1.pC = vec3(0.0f, 0.5f, 0.25f);
	t1.cA = Color(1, 0, 0, 1);
	t1.cB = Color(0, 1, 0, 1);
	t1.cC = Color(0, 0, 1, 1);
	tris.push_back(t1);

	t1.pA = vec3(-0.7f, 0.5f, 0.75f);
	t1.pB = vec3(0.0f, -0.5f, 0.0f);
	t1.pC = vec3(-0.2f, 0.75f, 0.75f);
	t1.cA = Color(1, 1, 0, 1);
	t1.cB = Color(0, 1, 1, 1);
	t1.cC = Color(1, 0, 1, 1);
	tris.push_back(t1);

	// Initialize buffer at black and 1.0f depth
	for (int y = 0; y < image.height(); y++)
	{
		for (int x = 0; x < image.width(); x++)
		{
			image(x, y) = Color(0, 0, 0, 1);
		}
	}

	// Draw triangles
	// aire signée de p0, p1, p2 = 0.5 *[(x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0)]
	for (int y = 0; y < image.height(); y++)
	{
		for (int x = 0; x < image.width(); x++)
		{
			for (int i = 0; i < tris.size(); i++)
			{
				Triangle t = tris[i];
				vec2 pX = vec2(x / (float)image.width() * 2.0f - 1.0f, y / (float)image.height() * 2.0f - 1.0f);

				float abx = 0.5f * ((t.pB.x - t.pA.x) * (pX.y - t.pA.y) - (pX.x - t.pA.x) * (t.pB.y - t.pA.y));
				float bcx = 0.5f * ((t.pC.x - t.pB.x) * (pX.y - t.pB.y) - (pX.x - t.pB.x) * (t.pC.y - t.pB.y));
				float cax = 0.5f * ((t.pA.x - t.pC.x) * (pX.y - t.pC.y) - (pX.x - t.pC.x) * (t.pA.y - t.pC.y));
				float abc = abx + bcx + cax;

				vec3 baryCoords = vec3(bcx / abc, cax / abc, abx / abc);
				float depth = t.pA.z * baryCoords.x + t.pB.z * baryCoords.y + t.pC.z * baryCoords.z;
				Color color = t.cA * baryCoords.x + t.cB * baryCoords.y + t.cC * baryCoords.z;

				if (abx >= 0.0f && bcx >= 0.0f && cax >= 0.0f && depth < image(x, y).a)
					image(x, y) = Color(color, depth);
			}
		}
	}
	write_image(image, "out.bmp");

	return 0;
}
