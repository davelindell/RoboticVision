#include "stdafx.h"
#include "Mnm.h"


Mnm::Mnm()
{
}


Mnm::~Mnm()
{
}

Mnm::Color Mnm::getColor(double pixel_val) {
	Mnm::Color return_val;
	//if ((0 < pixel_val && pixel_val < 25) || (135 < pixel_val && pixel_val < 180)){
	//	return_val = Mnm::blue;
	//}
	//else if (25 < pixel_val && pixel_val < 60) {
	//	return_val = Mnm::green;
	//}
	//else if (60 < pixel_val && pixel_val < 100) {
	//	return_val = Mnm::yellow;
	//}
	//else if (100 < pixel_val && pixel_val < 135) {
	//	return_val = Mnm::red;
	//}
	//else
	//	return_val = Mnm::unknown;
	//return return_val;
	if ((5 < pixel_val && pixel_val < 15)){
		return_val = Mnm::blue;
	}
	else if (42 < pixel_val && pixel_val < 48) {
		return_val = Mnm::green;
	}
	else if (71 < pixel_val && pixel_val < 77) {
		return_val = Mnm::yellow;
	}
	else if (117 < pixel_val && pixel_val < 125) {
		return_val = Mnm::red;
	}
	else
		return_val = Mnm::unknown;
	return return_val;

}
