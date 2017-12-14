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


//! representation d'un rayon.
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

//! representation d'un point d'intersection.
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


//! representation d'une source de lumiere.
struct Source : public Triangle
{
	Color emission;     //! flux emis.

	Source() : Triangle(), emission() {}
	Source(const TriangleData& data, const Color& color) : Triangle(data), emission(color) {}
};

// ensemble de sources de lumieres
std::vector<Source> sources;

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


// ensemble de triangles
std::vector<Triangle> triangles;

// recuperer les triangles du mesh
int build_triangles(const Mesh &mesh)
{
	for (int i = 0; i < mesh.triangle_count(); i++)
		triangles.push_back(Triangle(mesh.triangle(i)));

	printf("%d triangles.\n", (int)triangles.size());
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


// récupère la couleur du triangle touché
Color hitColor(Mesh& mesh, Hit& hit)
{
	Material mat = mesh.triangle_material(hit.object_id);
	return mat.diffuse + mat.emission;
}


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

	// relire une camera
	Orbiter camera;
	camera.lookat(Point(0, 0, 0), 4.0f);
	//camera.read_orbiter("m2tp/TutoRayTrace/orbiter.txt");

	// placer une source de lumiere
	Point light = camera.position();
	//Point light = Point(0.0f, 5.0f, 0.0f);
	float lightRadius = 6.0f;

	// creer l'image pour stocker le resultat
	Image image(512, 512);

	Transform meshRotation = Rotation(Vector(0, 1, 0), 8.0f);
	Transform meshTRS =  Scale(1.6f, 1.0f, 1.0f) * meshRotation * Translation(0, -1.0f, 0.75f);
	Transform worldToMesh = meshTRS.inverse();
	Transform worldToMeshRotation = meshRotation.inverse();

	Transform inverseProj = camera.projection(1024.0f, 640.0f, 60.0f).inverse();
	Transform inverseView = camera.view().inverse();
	Transform view = camera.view();
	Transform projToWorld = inverseView * inverseProj;

	// multi thread avec OpenMP
#pragma omp parallel for schedule(dynamic, 16)
	for (int y = 0; y < image.height(); y++)
	{
		for (int x = 0; x < image.width(); x++)
		{
			vec2 uv = vec2(x / (float)image.width(), y / (float)image.height());
			vec4 temp = vec4(uv.x * 2.0f - 1.0f, uv.y * 2.0f - 1.0f, 1.0f, 1.0f);
			temp = inverseProj(temp);
			temp.x = temp.x / temp.w;
			temp.y = temp.y / temp.w;
			temp.z = temp.z / temp.w;
			temp.w = temp.w / temp.w;
			temp = inverseView(temp);

			Point o = camera.position();	// origine du rayon
			Point e = Point(temp.x, temp.y, temp.z);	// extremite du rayon

			o = worldToMesh(o);
			e = worldToMesh(e);

			Ray ray(o, e);
			Hit hit;
			if (intersect(ray, hit))
			{
				// calculer l'eclairage direct pour chaque source
				Vector lightDir = normalize(hit.p - worldToMesh(light));
				float diffuseTerm = std::max(dot(-lightDir, hit.n), 0.0f);

				Color direct = hitColor(mesh, hit) * diffuseTerm;
				image(x, y) = Color(direct, 1);

				/*normal.x = normal.x < 0.0f ? 0.0f : normal.x;
				normal.y = normal.y < 0.0f ? 0.0f : normal.y;
				normal.z = normal.z < 0.0f ? 0.0f : normal.z;
				image(x, y) = Color(normal.x, normal.y, normal.z, 1);*/
			}
		}
	}

	write_image(image, "m2tp/TutoRayTrace/render.png");
	write_image_hdr(image, "m2tp/TutoRayTrace/render.hdr");
	return 0;
}
