#include "InpaintTest.h"
#include <QtWidgets/QApplication>

void GetGaussianKernel( double *gaus, int pixel, const double radius, const double sigma )
{
	double	sum		= 0;
	for ( int i = 0; i < pixel; i++ )
	{
		double	x = ( i * 2.0 / ( pixel - 1 ) - 1.0 ) * radius;
		for ( int j = 0; j < pixel; j++ )
		{
			double	y = ( j * 2.0 / ( pixel - 1 ) - 1.0 ) * radius;
			gaus[i * pixel + j] =
				exp( -( x * x + y * y ) / ( 2 * sigma * sigma ) );

			sum+=gaus[i * pixel + j];
		}
	}

	for ( int i=0; i < pixel; i++ )
	{
		for ( int j = 0; j < pixel; j++ )
		{
			gaus[i * pixel + j]/=sum;
		}
	}
	return;
}

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);


	int32_t	width	= 5;
	for ( int32_t x = -10; x < 20; ++x )
	{
		int32_t	xd	= abs(x) % ( width * 2 - 2 );
		qDebug() << x << "->" << xd - ( ( ( xd + 1 ) % width  ) * ( xd / width * 2 ) );
	}

	InpaintTest w;
	w.show();
	return a.exec();
}
