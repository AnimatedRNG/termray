#include "stdio.h"
#include "stdlib.h"
#include "math.h"
#include "string.h"
#include "unistd.h"

#define CO(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))
#define EPS 1.0e-9

typedef struct {
    double x;
    double y;
    double z;
} Vec3f;

typedef struct {
    Vec3f p0;
    Vec3f p1;
} Ray;

typedef struct {
    Vec3f center;
    double radius;
} Sphere;

typedef struct {
    Vec3f position;
    Vec3f color;
} Light;

Vec3f add_vec(Vec3f a, Vec3f b) {
    Vec3f result = {a.x + b.x, a.y + b.y, a.z + b.z};
    return result;
}

Vec3f sub_vec(Vec3f a, Vec3f b) {
    Vec3f result = {a.x - b.x, a.y - b.y, a.z - b.z};
    return result;
}

Vec3f scale_vec(Vec3f a, double factor) {
    Vec3f result = {a.x * factor, a.y * factor, a.z * factor};
    return result;
}

double vec_mag(Vec3f a) {
    return sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
}

Vec3f norm_vec(Vec3f a) {
    Vec3f result = scale_vec(a, 1.0 / vec_mag(a));
    return result;
}

double vec_dot_product(Vec3f a, Vec3f b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

int ray_sphere_intersection(Vec3f* result,
                            Ray ray,
                            Sphere sphere) {
    double dx = ray.p1.x - ray.p0.x;
    double dy = ray.p1.y - ray.p0.y;
    double dz = ray.p1.z - ray.p0.z;

    double a = dx * dx + dy * dy + dz * dz;
    double b = 2 * dx * (ray.p0.x - sphere.center.x) +
               2 * dy * (ray.p0.y - sphere.center.y) +
               2 * dz * (ray.p0.z - sphere.center.z);
    double c = sphere.center.x * sphere.center.x +
               sphere.center.y * sphere.center.y +
               sphere.center.z * sphere.center.z +
               ray.p0.x * ray.p0.x + ray.p0.y * ray.p0.y + ray.p0.z * ray.p0.z +
               -2 * (sphere.center.x * ray.p0.x +
                     sphere.center.y * ray.p0.y +
                     sphere.center.z * ray.p0.z) - sphere.radius * sphere.radius;

    double discriminant = b * b - 4 * a * c;
    double t = (-b - sqrt(b * b - 4 * a * c)) / (2 * a);
    if (discriminant < 0 || t < 0)
        return 0;
    else {
        result->x = ray.p0.x + t * dx;
        result->y = ray.p0.y + t * dy;
        result->z = ray.p0.z + t * dz;
        return 1;
    }
}

void ray_trace(Vec3f* result,
               Ray ray,
               Sphere* spheres,
               Light* lights,
               size_t cn, size_t ln) {
    double closest_z = -1;
    for (size_t i = 0; i < cn; i++) {
        Sphere sphere = spheres[i];

        int intersection = 0;
        Vec3f intersection_point = {0, 0, 0};
        if (ray_sphere_intersection(&intersection_point,
                                    ray,
                                    sphere)) {
            if (intersection_point.z > closest_z && closest_z != -1) {
                continue;
            } else {
                closest_z = intersection_point.z;
                intersection = 1;

                result->x = 0;
                result->y = 0;
                result->z = 0;
            }
        }

        for (size_t a = 0; a < ln; a++) {
            Light light_a = lights[a];

            if (intersection) {
                int in_shadow = 0;
                for (size_t b = 0; b < cn; b++) {
                    if (b == i)
                        continue;

                    Sphere sphere_b = spheres[b];

                    Vec3f dump;
                    Ray shadow_ray = {intersection_point, light_a.position};
                    if (ray_sphere_intersection(&dump,
                                                shadow_ray,
                                                sphere_b)) {
                        in_shadow = 1;
                        break;
                    }
                }

                if (in_shadow)
                    continue;

                Vec3f normal = norm_vec(sub_vec(intersection_point,
                                                sphere.center));

                Vec3f light_vec = norm_vec(sub_vec(intersection_point,
                                                   light_a.position));

                double dot_product = -vec_dot_product(normal, light_vec);
                if (dot_product > 0) {
                    *result = add_vec(*result,
                                      scale_vec(light_a.color, dot_product));

                    double max_val = 1.0 - EPS;
                    if (result->x >= max_val)
                        result->x = max_val;
                    if (result->y >= max_val)
                        result->y = max_val;
                    if (result->z >= max_val)
                        result->z = max_val;
                }
            }
        }
    }
}

int main(int argc, char** argv) {
    int width = 80, height = 55;
    float aspect_ratio = 2.5;
    if (argc == 4) {
        aspect_ratio = atof(argv[1]);
        int char_w = atoi(argv[2]);
        int char_h = atoi(argv[3]) - 1;
        width = char_w, height = char_h * aspect_ratio;
    }
    if ((argc > 1 && argc != 4) ||
            (width < 5 || width > 10000 ||
             height < 5 || height > 10000 ||
             aspect_ratio < 0.2 || aspect_ratio > 10)) {
        fprintf(stderr, "Invalid resolution!");
        return 1;
    }

    write(1, "\033[0;0H", 8);
    double z1 = 20;

    Vec3f origin = {0, 0, -30};

    Sphere spheres[] = {
        {{0., 0., 50.}, 40.},
        {{0., 0., 0.}, 2.}
    };

    Light lights[] = {
        {{0, 0, 0}, {0, 0.9, 0.1}},
        {{0, -50, -60}, {0.15, 0.05, 0.8}},
        {{0, 50, -60}, {0.25, 0.05, 0.7}}
    };

    double* result_buffer = malloc(height * width * 3 * sizeof(double));
    char* buffer = malloc(10000000);
    size_t index = 0;
    for (double t = 0; 1; t += 0.1) {
        #pragma omp parallel for
        for (int y1_i = -height / 2; y1_i < height / 2; y1_i++) {
            for (int x1_i = -width / 2; x1_i < width / 2; x1_i++) {
                Vec3f rgb = {0, 0, 0};

                double y1 = y1_i, x1 = x1_i;

                Ray primary_ray = {origin, {x1, y1, z1}};

                ray_trace(&rgb, primary_ray, spheres, lights,
                          CO(spheres), CO(lights));
                int offset = ((y1 + (height / 2)) * width +
                              (x1 + (width / 2))) * 3;
                result_buffer[offset] = rgb.x;
                result_buffer[offset + 1] = rgb.y;
                result_buffer[offset + 2] = rgb.z;
            }
        }

        for (float y = 0; y < height; y += aspect_ratio) {
            for (float x = 0; x < width; x++) {
                int offset1 = ((int) y * width + x) * 3;
                int offset2 = (((int) y + 1) * width + x) * 3;
                unsigned int r1 = (unsigned int)
                                  (result_buffer[offset1] * 256.);
                unsigned int g1 = (unsigned int)
                                  (result_buffer[offset1 + 1] * 256.);
                unsigned int b1 = (unsigned int)
                                  (result_buffer[offset1 + 2] * 256.);
                unsigned int r2 = (unsigned int)
                                  (result_buffer[offset2] * 256.);
                unsigned int g2 = (unsigned int)
                                  (result_buffer[offset2 + 1] * 256.);
                unsigned int b2 = (unsigned int)
                                  (result_buffer[offset2 + 2] * 256.);

                char* formatter =
                    "\x1b[38;2;%d;%d;%dm\x1b[48;2;%d;%d;%dm▀\x1b[0m ";
                size_t len = snprintf(NULL, 0, formatter,
                                      r1, g1, b1, r2, g2, b2);
                snprintf(&(buffer[index]), len, formatter,
                         r1, g1, b1, r2, g2, b2);

                index += len;
            }
            buffer[index] = '\n';
            index += 1;
        }

        usleep(10000);
        write(1, buffer, index);
        index = 0;
        write(1, "\033[0;0H", 8);

        lights[0].position.y = sin(t) * 7;
        lights[0].position.x = cos(t) * 5;

        spheres[1].center.x = sin(t / 4) * 45;
        spheres[1].center.y = cos(t * 2) * 3;
        spheres[1].center.z = cos(t / 4) * 45 + 50;
    }
}
