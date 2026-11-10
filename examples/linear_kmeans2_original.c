float linear_kmeans2(float* points, float* clusters) {
  unsigned long step = 0;
  int ownership[POINTS_LEN];
  float distance0[POINTS_LEN], distance1[POINTS_LEN];
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
  unsigned long num_cluster1_points = 0;
  for(unsigned long i = 0; i < POINTS_LEN; i++) {
    if(ownership[i]) { sum1 += powf(clusters[1]-points[i], 2);
                       num_cluster1_points++; }
    else             { sum0 += powf(clusters[0]-points[i], 2); }
  }
  unsigned long num_cluster0_points =
      POINTS_LEN - num_cluster1_points;
  sum0 /= num_cluster0_points;
  sum1 /= num_cluster1_points;
  if(isnanf(sum0)) sum0 = 0;
  if(isnanf(sum1)) sum1 = 0;
  return (sum0 + sum1) / 2;
}
