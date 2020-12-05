#include "Miracle3D.h"
#include <math.h>

vector3d quat2vec(quaternion3d q)
{
	vector3d result;

	float ysqr = q.y * q.y;

	float t0 = +2.0f * (q.w * q.x + q.y * q.z);
	float t1 = +1.0f - 2.0f * (q.x * q.x + ysqr);
	float roll = atan2(t0, t1);

	float t2 = +2.0f * (q.w * q.y - q.z * q.x);
	t2 = ((t2 > 1.0f) ? 1.0f : t2);
	t2 = ((t2 < -1.0f) ? -1.0f : t2);

	float pitch = asin(t2);
	float t3 = +2.0f * (q.w * q.z + q.x * q.y);
	float t4 = +1.0f - 2.0f * (ysqr + q.z * q.z);
	float yaw = atan2(t3, t4);

	result.x = roll / M_PI * 180;
	result.y = pitch / M_PI * 180;
	result.z = yaw / M_PI * 180;
	return result;
}

quaternion3d vec2quat(vector3d v)
{
	float cy = cos(v.z * 0.5);
	float sy = sin(v.z * 0.5);
	float cp = cos(v.y * 0.5);
	float sp = sin(v.y * 0.5);
	float cr = cos(v.x * 0.5);
	float sr = sin(v.x * 0.5);

	quaternion3d q;
	q.w = cr * cp * cy + sr * sp * sy;
	q.x = sr * cp * cy - cr * sp * sy;
	q.y = cr * sp * cy + sr * cp * sy;
	q.z = cr * cp * sy - sr * sp * cy;



	return q;
}

bool areVecsEqual(vector3d v1, vector3d v2)
{
	if (fabs(v1.x - v2.x) < 0.1f &&
		(fabs(v1.y - v2.y) < 0.1f) &&
		(fabs(v1.z - v2.z) < 0.1f))
		return true;
	else
		return false;
}

bool areVertsEqual(v_data v1, v_data v2)
{
	if (fabs(v1.vert.x - v2.vert.x) < 0.01f &&
		(fabs(v1.vert.y - v2.vert.y) < 0.01f) &&
		(fabs(v1.vert.z - v2.vert.z) < 0.01f))
		return true;
	else
		return false;
}


