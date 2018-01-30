#include <cfloat>
#include <cmath>
#include <time.h>
#include <algorithm>

#include "vec.h"
#include "color.h"
#include "mat.h"
#include "mesh.h"
#include "wavefront.h"
#include "image.h"
#include "image_io.h"
#include "image_hdr.h"
#include "orbiter.h"

#define EPSILON 0.00001f

using namespace std;

// Structures
struct Ray
{
	Point o;	//!< origine.
	Vector d;	//!< direction.
	float tmax;	//!< abscisse max pour les intersections valides.

	Ray(const Point origine, const Point extremite) : o(origine), d(Vector(origine, extremite)), tmax(1) {}
	Ray(const Point origine, const Vector direction) : o(origine), d(direction), tmax(FLT_MAX) {}

	//!	renvoie le point a l'abscisse t sur le rayon
	Point operator( ) (const float t) const { return o + t * d; }
};
struct Hit
{
	Point p;	    //!< position.
	Vector n;	    //!< normale.
	float t;	    //!< t, abscisse sur le rayon.
	float u, v;	    //!< u, v coordonnees barycentrique dans le triangle.
	int object_id;  //! indice du triangle dans le maillage.

	Hit() : p(), n(), t(FLT_MAX), u(0), v(0), object_id(-1) {}
};
struct Triangle : public TriangleData
{
	Triangle() : TriangleData() {}
	Triangle(const TriangleData& data) : TriangleData(data) {}

	/* calcule l'intersection ray/triangle
	cf "fast, minimum storage ray-triangle intersection"
	http://www.graphics.cornell.edu/pubs/1997/MT97.pdf

	renvoie faux s'il n'y a pas d'intersection valide, une intersection peut exister mais peut ne pas se trouver dans l'intervalle [0 htmax] du rayon. \n
	renvoie vrai + les coordonnees barycentriques (ru, rv) du point d'intersection + sa position le long du rayon (rt). \n
	convention barycentrique : t(u, v)= (1 - u - v) * a + u * b + v * c \n
	*/
	bool intersect(const Ray &ray, const float htmax, float &rt, float &ru, float&rv) const
	{
		/* begin calculating determinant - also used to calculate U parameter */
		Vector ac = Vector(Point(a), Point(c));
		Vector pvec = cross(ray.d, ac);

		/* if determinant is near zero, ray lies in plane of triangle */
		Vector ab = Vector(Point(a), Point(b));
		float det = dot(ab, pvec);
		if (det > -EPSILON && det < EPSILON)
			return false;

		float inv_det = 1.0f / det;

		/* calculate distance from vert0 to ray origin */
		Vector tvec(Point(a), ray.o);

		/* calculate U parameter and test bounds */
		float u = dot(tvec, pvec) * inv_det;
		if (u < 0.0f || u > 1.0f)
			return false;

		/* prepare to test V parameter */
		Vector qvec = cross(tvec, ab);

		/* calculate V parameter and test bounds */
		float v = dot(ray.d, qvec) * inv_det;
		if (v < 0.0f || u + v > 1.0f)
			return false;

		/* calculate t, ray intersects triangle */
		rt = dot(ac, qvec) * inv_det;
		ru = u;
		rv = v;

		// ne renvoie vrai que si l'intersection est valide (comprise entre tmin et tmax du rayon)
		return (rt <= htmax && rt > EPSILON);
	}

	//! renvoie l'aire du triangle
	float area() const
	{
		return length(cross(Point(b) - Point(a), Point(c) - Point(a))) / 2.f;
	}

	//! renvoie un point a l'interieur du triangle connaissant ses coordonnees barycentriques.
	//! convention p(u, v)= (1 - u - v) * a + u * b + v * c
	Point point(const float u, const float v) const
	{
		float w = 1.f - u - v;
		return Point(Vector(a) * w + Vector(b) * u + Vector(c) * v);
	}

	//! renvoie une normale a l'interieur du triangle connaissant ses coordonnees barycentriques.
	//! convention p(u, v)= (1 - u - v) * a + u * b + v * c
	Vector normal(const float u, const float v) const
	{
		float w = 1.f - u - v;
		return Vector(na) * w + Vector(nb) * u + Vector(nc) * v;
	}
};
struct Source : public Triangle
{
	Color emission;     //! flux emis.

	Source() : Triangle(), emission() {}
	Source(const TriangleData& data, const Color& color) : Triangle(data), emission(color) {}
};
struct AABB
{
public:
	Point minPoint = Point(0, 0, 0);
	Point maxPoint = Point(0, 0, 0);

	AABB() { }

	bool intersect(const Ray& ray, const float htmax, float& rtmin, float& rtmax) const
	{
		Vector invd = Vector(1.f / ray.d.x, 1.f / ray.d.y, 1.f / ray.d.z);
		// remarque : il est un peu plus rapide de stocker invd dans la structure Ray, ou dans l'appellant / algo de parcours, au lieu de la recalculer à chaque fois

		Point rmin = minPoint;
		Point rmax = maxPoint;
		if (ray.d.x < 0) std::swap(rmin.x, rmax.x);    // le rayon entre dans la bbox par pmax et ressort par pmin, echanger...
		if (ray.d.y < 0) std::swap(rmin.y, rmax.y);
		if (ray.d.z < 0) std::swap(rmin.z, rmax.z);

		Vector dmin = (rmin - ray.o) * invd;           // intersection avec les plans -U -V -W attachés à rmin
		Vector dmax = (rmax - ray.o) * invd;           // intersection avec les plans +U +V +W attachés à rmax
		rtmin = std::max(dmin.x, std::max(dmin.y, std::max(dmin.z, 0.f)));        // borne min de l'intervalle d'intersection
		rtmax = std::min(dmax.x, std::min(dmax.y, std::min(dmax.z, htmax)));      // borne max

																				  // ne renvoie vrai que si l'intersection est valide (l'intervalle n'est pas degenere)
		return (rtmin <= rtmax);
	}
};
struct Primitive
{
	AABB bounds;
	Point center;
	int triangleId;
};
struct BVHNode
{
public:
	AABB aabb;
	int leftId;
	int rightId;
	int triangleId;

	BVHNode(const AABB& b) : aabb(b), leftId(-1), rightId(-1), triangleId(-1) { }
	BVHNode(const AABB& b, const int& l, const int& r, const int& t) : aabb(b), leftId(l), rightId(r), triangleId(t) { }
};

// Global variables
vector<Source> sources;
vector<Triangle> triangles;
vector<Primitive> primitives;
vector<BVHNode> bvh;
int rootNodeId = 0;
float goldenNumber = (sqrt(5.0f) + 1.0f) / 2.0f;

// Tools
Point min(const Point& a, const Point& b)
{
	return Point(min(a.x, b.x), min(a.y, b.y), min(a.z, b.z));
}
Point max(const Point& a, const Point& b)
{
	return Point(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z));
}
Vector min(const Vector& a, const Vector& b)
{
	return Vector(min(a.x, b.x), min(a.y, b.y), min(a.z, b.z));
}
Vector max(const Vector& a, const Vector& b)
{
	return Vector(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z));
}


// recuperer les sources de lumiere du mesh : triangles associee a une matiere qui emet de la lumiere, material.emission != 0
int build_sources(const Mesh& mesh)
{
	for (int i = 0; i < mesh.triangle_count(); i++)
	{
		// recupere la matiere associee a chaque triangle de l'objet
		Material material = mesh.triangle_material(i);

		if ((material.emission.r + material.emission.g + material.emission.b) > 0)
			// inserer la source de lumiere dans l'ensemble.
			sources.push_back(Source(mesh.triangle(i), material.emission));
	}

	printf("%d sources.\n", (int)sources.size());
	return (int)sources.size();
}

// verifie que le rayon touche une source de lumiere.
bool direct(const Ray& ray)
{
	for (size_t i = 0; i < sources.size(); i++)
	{
		float t, u, v;
		if (sources[i].intersect(ray, ray.tmax, t, u, v))
			return true;
	}

	return false;
}

// recuperer les triangles du mesh
int build_triangles(const Mesh &mesh)
{
	for (int i = 0; i < mesh.triangle_count(); i++)
	{
		Triangle t(mesh.triangle(i));
		triangles.push_back(t);

		Primitive p;
		p.bounds.minPoint = min(min(Point(t.a), Point(t.b)), Point(t.c));
		p.bounds.maxPoint = max(max(Point(t.a), Point(t.b)), Point(t.c));
		p.center = Point((Vector(p.bounds.maxPoint) + Vector(p.bounds.minPoint)) / 2.0f);
		p.triangleId = i;

		primitives.push_back(p);
	}
	printf("%d triangles.\n", (int)triangles.size());
	printf("%d primitives.\n", (int)primitives.size());
	return (int)triangles.size();
}


// calcule l'intersection d'un rayon et de tous les triangles
bool intersect(const Ray& ray, Hit& hit)
{
	hit.t = ray.tmax;
	for (size_t i = 0; i < triangles.size(); i++)
	{
		float t, u, v;
		if (triangles[i].intersect(ray, hit.t, t, u, v))
		{
			hit.t = t;
			hit.u = u;
			hit.v = v;

			hit.p = ray(t);	// evalue la positon du point d'intersection sur le rayon
			hit.n = triangles[i].normal(u, v);

			hit.object_id = i;	// permet de retrouver toutes les infos associees au triangle
		}
	}

	return (hit.object_id != -1);
}

// Intersect scene using BVH
bool intersect(const Ray& ray, Hit& hit, int bvhId)
{
	float entryT, exitT;
	BVHNode node = bvh[bvhId];

	// Primitive
	if (node.triangleId != -1)
	{
		float v;
		int id = node.triangleId;
		if (triangles[id].intersect(ray, 1.0, entryT, exitT, v))
		{
			hit.t = entryT;
			hit.u = exitT;
			hit.v = v;

			hit.p = ray(entryT);	// evalue la positon du point d'intersection sur le rayon
			hit.n = triangles[id].normal(exitT, v);

			hit.object_id = id;	// permet de retrouver toutes les infos associees au triangle
			return true;
		}
		else
			return false;
	}

	// Explore
	if (node.aabb.intersect(ray, hit.t, entryT, exitT) == true)
	{
		bool left = intersect(ray, hit, node.leftId);
		bool right = intersect(ray, hit, node.rightId);
		return right || left;
	}
	return false;
}


// récupère la couleur du triangle touché
Color hitColor(Mesh& mesh, Hit& hit)
{
	Material mat = mesh.triangle_material(hit.object_id);
	return mat.diffuse + mat.emission;
}


// b1, b2, n sont 3 axes orthonormes.
void branchlessONB(const Vector &n, Vector &b1, Vector &b2)
{
	float sign = std::copysign(1.0f, n.z);
	const float a = -1.0f / (sign + n.z);
	const float b = n.x * n.y * a;
	b1 = Vector(1.0f + sign * n.x * n.x * a, sign * b, -sign * n.x);
	b2 = Vector(b, sign + n.y * n.y * a, -n.y);
}

// Ambient Occlusion
float GetAmbientOcclusionTerm(const Hit& origin, const int iterations)
{
	float accumulator = 0.0f;
	for (int i = 0; i < iterations; i++)
	{
		// Create fibonnaci vector
		float u = rand() / (float)RAND_MAX;
		float phi = 2.0f * M_PI * (((i + u) / goldenNumber) - floor((i + u) / goldenNumber));
		float cosTheta = 1.0f - ((2.0f * i + 1.0f) / (2.0f * iterations));
		float sinTheta = sqrt(1.0f - (cosTheta * cosTheta));
		Vector fiboDir(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

		// Convert to world space
		Vector tangent, binormal;
		branchlessONB(origin.n, tangent, binormal);
		Vector fiboWorldDir(fiboDir.x * tangent + fiboDir.y * binormal + fiboDir.z * origin.n);

		// Cast ray
		Hit hit;
		Ray ray(origin.p + 0.001f * origin.n, fiboDir);
		if (intersect(ray, hit, rootNodeId) == false)
			accumulator += dot(fiboDir, origin.n);
	}
	return accumulator / (float)iterations * M_PI;
}

// Bounding volume hierarchy
struct predicat
{
	int axe;
	float coupe;

	predicat(const int _axe, const float _coupe) : axe(_axe), coupe(_coupe) { }
	bool operator() (const Primitive& p) const
	{
		return (p.center(axe) < coupe);
	}
};

unsigned int build_nodes(vector<BVHNode>& nodes,
	vector<Primitive>& primitives,
	const unsigned int begin,
	const unsigned int end)
{
	if (end - begin <= 1)
	{
		// construire une feuille qui reference la primitive d'indice begin, et la boite englobante du triangle associee a la primitive...
		// renvoyer l'indice de la feuille
		nodes.push_back(BVHNode(primitives[begin].bounds, -1, -1, primitives[begin].triangleId));
		return nodes.size() - 1;
	}

	// Construire la boite englobante des centres des primitives d'indices [begin .. end[
	AABB b;
	for (unsigned int i = begin; i < end; i++)
	{
		b.minPoint = min(b.minPoint, primitives[i].center);
		b.maxPoint = max(b.maxPoint, primitives[i].center);
	}

	// Trouver l'axe le plus etire de la boite englobante
	// Couper en 2 au milieu de boite englobante sur l'axe le plus etire
	Vector temp(b.maxPoint - b.minPoint);
	float maxValue = max(max(temp.x, temp.y), temp.z);
	int axe = (maxValue == temp.x) ? 0 : (maxValue == temp.y) ? 1 : 2;
	float coupe = (b.minPoint(axe) + b.maxPoint(axe)) / 2.0f;

	// partitionner les primitives par rapport a la "coupe"
	Primitive* pmid = partition(primitives.data() + begin, primitives.data() + end, predicat(axe, coupe));
	unsigned int mid = distance(primitives.data(), pmid);

	// verifier que la partition n'est pas degeneree (toutes les primitives du meme cote de la separation)
	if (mid == begin || mid == end)
		mid = (begin + end) / 2;
	assert(mid != begin);
	assert(mid != end);
	// remarque : il est possible que 2 (ou plus) primitives aient le meme centre,
	// dans ce cas, la boite englobante des centres est reduite à un point, et la partition est forcement degeneree
	// une solution est de construire une feuille,
	// ou, autre solution, forcer une repartition arbitraire des primitives entre 2 les fils, avec mid= (begin + end) / 2

	// construire le fils gauche 
	unsigned int left = build_nodes(nodes, primitives, begin, mid);

	// construire le fils droit 
	unsigned int right = build_nodes(nodes, primitives, mid, end);

	// construire un noeud interne
	// quelle est sa boite englobante ?
	AABB nodeBox;
	nodeBox.minPoint = min(nodes[left].aabb.minPoint, nodes[right].aabb.minPoint);
	nodeBox.maxPoint = max(nodes[left].aabb.maxPoint, nodes[right].aabb.maxPoint);
	nodes.push_back(BVHNode(nodeBox, left, right, -1));

	// renvoyer l'indice du noeud
	return nodes.size() - 1;
}


// MAIN
int main(int argc, char **argv)
{
	// init generateur aleatoire
	srand(time(NULL));

	// lire un maillage et ses matieres	
	Mesh mesh = read_mesh("m2tp/TutoRayTrace/cornell.obj");
	if (mesh == Mesh::error())
		return 1;

	// extraire les sources
	build_sources(mesh);
	// extraire les triangles du maillage
	build_triangles(mesh);
	// Build the scene's BVH
	rootNodeId = build_nodes(bvh, primitives, 0, primitives.size());

	// relire une camera
	Orbiter camera;
	camera.lookat(Point(0, 1, 0), 4.0f);
	//camera.read_orbiter("m2tp/TutoRayTrace/orbiter.txt");

	// placer une source de lumiere
	Point light = camera.position();
	//Point light = Point(0.0f, 5.0f, 0.0f);
	float lightRadius = 6.0f;
	float fieldOfView = 60.0f;

	// creer l'image pour stocker le resultat
	Image image(512, 512);

	// multi thread avec OpenMP
#pragma omp parallel for schedule(dynamic, 16)
	for (int y = 0; y < image.height(); y++)
	{
		for (int x = 0; x < image.width(); x++)
		{
			if (x == image.width() / 2 - 150 && y == image.height() / 2)
			{
				int x = 0;
			}

			Point dO;
			Vector dx, dy;
			camera.frame(image.width(), image.height(), 1.0f, fieldOfView, dO, dx, dy);
			Point o = light;
			Point e = dO + x * dx + y * dy;
			Ray ray(o, e);
			Hit hit;
			hit.t = ray.tmax;
			if (intersect(ray, hit, rootNodeId) == true)
			{
				// calculer l'eclairage direct pour chaque source
				Vector lightDir = normalize(hit.p - light);
				float diffuseTerm = std::max(dot(-lightDir, hit.n), 0.0f);

				// Compute ambient occlusion factor
				float ambientTerm = 1;// GetAmbientOcclusionTerm(hit, 32);

				// Render result
				Color direct = hitColor(mesh, hit) * diffuseTerm * ambientTerm;
				image(x, y) = Color(direct, 1);
				//image(x, y) = Color(ambient, ambient, ambient, 1);
			}
		}
	}

	write_image(image, "m2tp/TutoRayTrace/render.png");
	write_image_hdr(image, "m2tp/TutoRayTrace/render.hdr");
	return 0;
}
