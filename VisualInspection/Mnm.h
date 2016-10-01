#pragma once
class Mnm
{
public:
	static enum Color { blue, green, yellow, red, unknown };
	Mnm();
	~Mnm();

	static Color getColor(double pixel_val);

};

