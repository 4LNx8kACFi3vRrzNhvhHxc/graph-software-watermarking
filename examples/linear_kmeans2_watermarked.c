float linear_kmeans2(float* points, float* clusters) {
  unsigned long step = 0;
  float distance0[POINTS_LEN], distance1[POINTS_LEN];
  int ownership[POINTS_LEN];
  do {
    unsigned long i = 0;
    do {
      distance0[i] = fabs(clusters[0] - points[i]);
    } while (++i < POINTS_LEN);
    i = 0;
    do {
      distance1[i] = fabs(clusters[1] - points[i]);
      ownership[i] = (distance1[i] < distance0[i]);
    } while (++i < POINTS_LEN);
    clusters[0] = clusters[1] = i = 0;
    while (++i < POINTS_LEN){
      clusters[ownership[i]] += points[i];
    };
    clusters[0] /= POINTS_LEN;
    clusters[1] /= POINTS_LEN;
  } while (++step < NUM_ITERATIONS);
  float sum0 = 0, sum1 = 0;
  unsigned long i = 0;
  unsigned long num_cluster1_points = 0;
  do {
    float d0 = clusters[0] - points[i];
    float d1 = clusters[1] - points[i];
    sum0 += (1 - ownership[i]) * powf(d0, 2);
    sum1 += ownership[i] * powf(d1, 2);
    num_cluster1_points += ownership[i];
  } while (++i < POINTS_LEN);
  unsigned long num_cluster0_points =
      POINTS_LEN - num_cluster1_points;
  sum0 /= num_cluster0_points + !num_cluster0_points;
  sum1 /= num_cluster1_points + !num_cluster1_points;
  return (sum0 + sum1) / 2;
}
