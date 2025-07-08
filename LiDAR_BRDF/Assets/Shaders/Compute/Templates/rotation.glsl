mat4 rotation3d(vec3 axis, float angle) {
  axis		 = normalize(axis);
  float s	= sin(angle);
  float c	= cos(angle);
  vec3 temp	= (1.0 - c) * axis;

  return mat4(
		c + temp[0] * axis[0], temp[1] * axis[0] - s * axis[2], temp[2] * axis[0] + s * axis[1],  0.0f,
		temp[0] * axis[1] + s * axis[2], c + temp[1] * axis[1], temp[2] * axis[1] - s * axis[0],  0.0f,
		temp[0] * axis[2] - s * axis[1], temp[1] * axis[2] + s * axis[0], c + temp[2] * axis[2],  0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);
}