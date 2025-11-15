#include "Help.h"

qreal metersToQreal(double meters)
{
	return static_cast<qreal>(meters) * SCALE;
}

double qrealToMeters(qreal value)
{
	return static_cast<double>(value / SCALE);
}
