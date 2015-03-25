
#include "Vec.inl"

int draw_data(Vec<3> *points, int num_particles, int num_points, double min_x, double min_y, double max_x, double max_y, double radius);

void read_input(bool verbose, FILE *fp, Vec<3> **points, int *num_particles, int *num_points, double *min_x, double *max_x, double *min_y, double *max_y, double *radius);

bool str_equals(char *a, char *b);
