
#include "genmap.h"
#include "OpenSimplexNoise.h"
#include <random>
#include <algorithm>

Color normalizeRGB(Vector3 val);
float normalize(float val, int min, int max);

Noisey::Noisey() {
	resetRiver();
	srand(time(NULL));
}

void Noisey::resetRiver() {

	for (int x = 0; x < 10000; x++) {
		for (int y = 0; y < 10000; y++)
			river[x][y] = None;
	}

}

void Noisey::_bind_methods() {
	ObjectTypeDB::bind_method("set_image", &Noisey::set_image);
	ObjectTypeDB::bind_method("set_exponent", &Noisey::set_exponent);
	ObjectTypeDB::bind_method("set_frequency", &Noisey::set_frequency);
	ObjectTypeDB::bind_method("get_map", &Noisey::get_map);
	ObjectTypeDB::bind_method("get_moistureMap", &Noisey::get_moistureMap);
	ObjectTypeDB::bind_method("get_elevationMap", &Noisey::get_elevationMap);
	ObjectTypeDB::bind_method("get_heatMap", &Noisey::get_heatMap);
	ObjectTypeDB::bind_method("get_riverMap", &Noisey::get_riverMap);
	ObjectTypeDB::bind_method("create_map", &Noisey::create_map);
}
float Noisey::constrainNoise(float val) {
	float constrainedVal = val;
	if (val > 1)
		constrainedVal = 1;
	else if (val < -1)
		constrainedVal = -1;

	return constrainedVal;
}

void Noisey::set_image(Image img) {
	terrainColours  = img;
}

void Noisey::set_exponent(float exp) {
	exponent = exp;
}

void Noisey::set_frequency(float freq) {
	frequency = freq;
}

bool Noisey::nearbySourceCheck(Vector2 xy, int mapsizex, int mapsizey) {

	for (int x = 0; x < mapsizex; x++){
		for (int y = 0; y < mapsizey; y++) {
			if (river[x][y] == Source) {
				float distance = 0;
				distance = Vector2(x, y).distance_to(xy);
				if (distance < ((mapsizex * mapsizey)/ 1000) ) {
					return false;
				}
			}
		}
	}
	return true;
}

void Noisey::create_map(int mapsizex, int mapsizey) {
	mapWidth = mapsizex;
	mapHeight = mapsizey;
	
	map.create(mapsizex, mapsizey, false, Image::FORMAT_RGBA);
	elevationMap.create(mapsizex, mapsizey, false, Image::FORMAT_GRAYSCALE);
	moistureMap.create(mapsizex, mapsizey, false, Image::FORMAT_RGBA);
	tempMap.create(mapsizex, mapsizey, false, Image::FORMAT_RGBA);
	riverMap.create(mapsizex, mapsizey, false, Image::FORMAT_RGBA);

	createNoise();
	createRivers();
	applyRiverMoisture();
	paintMap();

}

void Noisey::createNoise() {

	OpenSimplexNoise N = OpenSimplexNoise();
	OpenSimplexNoise M = OpenSimplexNoise(rand() % 999999 + 1);
	OpenSimplexNoise T = OpenSimplexNoise(rand() % 999999 + 1);
	printf("\nnoise Gen Started");

	for (int x = 0; x < mapWidth; x++) {
		for (int y = 0; y < mapHeight; y++) {
			double nx = x, ny = y;
			double e = N.Evaluate(nx * frequency / 100, ny * frequency / 100);
			e += 0.5 * N.Evaluate(nx * 2/100, ny * 2/100);
			e += 0.25 * N.Evaluate(nx * 4/100, ny * 4/100);
			e += 0.13 * N.Evaluate(nx * 8/100, ny * 8/100);
			e += 0.07 * N.Evaluate(nx * 16/100, ny * 16/100);
			e += 0.035 * N.Evaluate(nx * 32/100, ny * 32/100);

			e /= (0.5 + 0.25 + 0.13 + 0.07 + 0.035);

			float d = 2 * MAX(abs(nx), abs(ny));

			//if (isIsland)
			//noise[x][y] = e + a - b * pow(d, c);

			if (e < 0) {
				float temp = -e;
				noise[x][y] = pow(temp, exponent);
				noise[x][y] = -noise[x][y];
			}
			else
				noise[x][y] = pow(e, exponent);

			if (e < -1 || e > 1) {
				noise[x][y] = constrainNoise(e);
			}

		}
	}

	for (int x = 0; x < mapWidth; x++) {
		for (int y = 0; y < mapHeight; y++) {
			double nx = x, ny = y;
			double e = M.Evaluate(nx * frequency / 10, ny * frequency / 10);
			double t = T.Evaluate(nx * frequency / 2, ny * frequency / 2);

			t = sin(((pow((y + t), 1.03) - (e / 0.1)) / ((mapHeight / 4) - 5)) - 1) + 0.2; //apply moisture to heat
			e = e - (t*0.4); // apply heat on moisture

			float ha = 1;

			if (e > 1)
				e = 1;
			else if (e < -1)
				e = -1;

			if (t > 1)
				t = 1;
			else if (t < -1)
				t = -1;

			moisture[x][y] = e;
			temperature[x][y] = t;
		}
	}

}

void Noisey::createRivers() {

	printf("\nriver Gen started");

	resetRiver();

	for (int x = 0; x < mapWidth; x++) {
		for (int y = 0; y < mapHeight; y++) {
			if (noise[x][y] > 0.5 && nearbySourceCheck(Vector2(x, y), mapWidth, mapHeight) && temperature[x][y] > 0) {
				//start river creation from this point
				float nPV = 1;
				float cPV = 1;
				Vector2 startPoint = Vector2(x, y);
				int cPtx = startPoint.x;
				int cPty = startPoint.y;
				int tPtx = 0;
				int tPty = 0;
				bool stop = false;

				std::vector<Vector2> usedPoints;

				float maxIterations = moisture[x][y] * 200;
				int currIteration = 0;
				if (maxIterations > 0) {
					riverMap.put_pixel(x, y, Color(0, 0.2 * normalize(noise[x][y], -1, 1), 0.9 * normalize(noise[x][y], -1, 1)));
					river[x][y] = Source;
				}

				while (nPV > 0 && stop == false) {
					tPtx = cPtx;
					tPty = cPty;
					cPV = nPV;
					nPV = noise[cPtx][cPty];

					Vector<Vector3> points;
					//up

					if (cPty - 1 >= 0) {
						if (std::find(usedPoints.begin(), usedPoints.end(), Vector2(cPtx, cPty - 1)) == usedPoints.end()) {
							points.push_back(Vector3(cPtx, cPty - 1, noise[cPtx][cPty - 1]));
						}
					}
					//down
					if (cPty + 1 < mapHeight) {
						if (std::find(usedPoints.begin(), usedPoints.end(), Vector2(cPtx, cPty + 1)) == usedPoints.end()) {
							points.push_back(Vector3(cPtx, cPty + 1, noise[cPtx][cPty + 1]));
						}
					}
					//right
					if (cPtx + 1 < mapWidth) {
						if (std::find(usedPoints.begin(), usedPoints.end(), Vector2(cPtx + 1, cPty)) == usedPoints.end()) {
							points.push_back(Vector3(cPtx + 1, cPty, noise[cPtx + 1][cPty]));
						}
					}
					//left
					if (cPtx - 1 >= 0) {
						if (std::find(usedPoints.begin(), usedPoints.end(), Vector2(cPtx - 1, cPty)) == usedPoints.end()) {
							points.push_back(Vector3(cPtx - 1, cPty, noise[cPtx - 1][cPty]));
						}
					}
					float minPointIndex = -1;
					float pointf = 1000;
					for (int i = 0; i < points.size(); i++) {

						if (points[i].z < pointf) {
							pointf = points[i].z;
							minPointIndex = i;
						}
					}

					if (minPointIndex != -1) {
						int minPointx = points[minPointIndex].x;
						int minPointy = points[minPointIndex].y;
						riverMap.put_pixel(minPointx, minPointy, Color(0, 0.2 * normalize(noise[minPointx][minPointy], -1, 1), 0.9 * normalize(noise[minPointx][minPointy], -1, 1)));
						river[minPointx][minPointy] = Water;

						nPV = pointf;

						cPtx = minPointx;
						cPty = minPointy;

						usedPoints.push_back(Vector2(cPtx, cPty));
					}

					nPV = pointf;

					if (cPtx == tPtx && cPty == tPty)
						stop = true;

					currIteration++;
				}
			}
		}
	}

}

void Noisey::applyRiverMoisture() {

	printf("\napplying moisture from rivers started");

	int range = (mapWidth*mapHeight) / 10000; // small optimisation so its not checking ever tile

	for (int x = 0; x < mapWidth; x++) {
		for (int y = 0; y < mapHeight; y++) {
			if (river[x][y] == Water) {
				for (int i = x - range; i < x + range; i++) {
					for (int j = y - range; j < y + range; j++) {
						float distance;
						distance = Vector2(x, y).distance_to(Vector2(i, j));
						if (distance < (mapWidth*mapHeight) / 10000) {
							if (moisture[i][j] < 0)
								moisture[i][j] = -moisture[i][j];

							moisture[i][j] = pow(moisture[i][j], 0.5);
						}
					}
				}
			}
		}
	}

}

void Noisey::paintMap() {

	printf("\nmap creation started");

	for (int x = 0; x < mapWidth; x++) {
		for (int y = 0; y < mapHeight; y++) {
			if (river[x][y] == None || temperature[x][y] < 0) {
				map.put_pixel(x, y, get_col(noise[x][y], moisture[x][y], temperature[x][y], x, y));
			}
			else map.put_pixel(x, y, Color(0, 0.670 * normalize(noise[x][y], -1, 1), 0.941 * normalize(noise[x][y], -1, 1)));
			elevationMap.put_pixel(x, y, Color(normalize(noise[x][y], -1, 1), normalize(noise[x][y], -1, 1), normalize(noise[x][y], -1, 1), 0.5));
			moistureMap.put_pixel(x, y, Color(normalize(moisture[x][y], -1, 1), normalize(moisture[x][y], -1, 1), 1));
			tempMap.put_pixel(x, y, Color(1, normalize(temperature[x][y], -1, 1), normalize(temperature[x][y], -1, 1), 1));
		}
	}

}

Image Noisey::get_map() {
	return map;
}

Image Noisey::get_moistureMap() {
	return moistureMap;
}

Image Noisey::get_elevationMap() {
	return elevationMap;
}

Image Noisey::get_heatMap() {
	return tempMap;
}

Image Noisey::get_riverMap() {
	return riverMap;
}

Color Noisey::get_col(float noiseValue, float moistureValue, float tempValue, int x, int y) {

	//printf("\ngetting col for pixel");


	if (tempValue < -0.1 && (y < rand() % 7 + 2 || y >  rand() % (mapHeight - (mapHeight - 10)) + (mapHeight - 10))) {
		return type_to_col(Polar_Ice, (noiseValue <= 0)? 1 : noiseValue, x, y);
	}

	if (noiseValue <= -0.2) {
		return type_to_col(Ocean, noiseValue, x, y);
	}

	else if (noiseValue <= 0) {
		return type_to_col(ShallowWater, noiseValue, x, y);
	}

	else if (noiseValue <= 0.05) {
		if (moistureValue <= 0.1)
			return type_to_col(Beach, noiseValue, x, y);

		if (moistureValue <= 0.2)
		{
			if (tempValue <= 0)
				return type_to_col(Tundra, noiseValue, x, y);
			if (tempValue <= 0.2)
				return type_to_col(Taiga, noiseValue, x, y);
			if (tempValue <= 0.6)
				return type_to_col(Chaparral, noiseValue, x, y);
			if (tempValue <= 0.8)
				return type_to_col(Savanna, noiseValue, x, y);
			if (tempValue <= 1)
				return type_to_col(Desert, noiseValue, x, y);
		}

		if (moistureValue <= 0.5)
		{
			if (tempValue <= 0)
				return type_to_col(Taiga, noiseValue, x, y);
			
			return type_to_col(TemperateForest, noiseValue, x, y);
		}

		if (moistureValue <= 1)
		{
			if (tempValue <= 0)
				return type_to_col(Taiga, noiseValue, x, y);
			if (tempValue <= 0.5)
				return type_to_col(TemperateRainForest, noiseValue, x, y);

			return type_to_col(TropicalRainForest, noiseValue, x, y);
		}
	}


	else if (noiseValue <= 0.5) {
		if (moistureValue <= 3)
		{
			if (tempValue <= 0)
				return type_to_col(Tundra, noiseValue, x, y);
			if (tempValue <= 0.2)
				return type_to_col(Taiga, noiseValue, x, y);
			if (tempValue <= 0.6)
				return type_to_col(Chaparral, noiseValue, x, y);
			if (tempValue <= 0.8)
				return type_to_col(Savanna, noiseValue, x, y);
			if (tempValue <= 1)
				return type_to_col(Desert, noiseValue, x, y);
		}

		if (moistureValue <= 0.6)
		{
			if (tempValue <= 0.1)
				return type_to_col(Taiga, noiseValue, x, y);

			return type_to_col(TemperateForest, noiseValue, x, y);
		}

		if (moistureValue <= 1)
		{
			if (tempValue <= 0.2)
				return type_to_col(Taiga, noiseValue, x, y);
			if (tempValue <= 0.7)
				return type_to_col(TemperateRainForest, noiseValue, x, y);

			return type_to_col(TropicalRainForest, noiseValue, x, y);
		}
	}

	else if (noiseValue <= 1) {
		if (moistureValue <= 0.6)
		{
			if (tempValue <= 0)
				return type_to_col(Tundra, noiseValue, x, y);
			if (tempValue <= 0.2)
				return type_to_col(Taiga, noiseValue, x, y);
			if (tempValue <= 0.6)
				return type_to_col(Chaparral, noiseValue, x, y);
			if (tempValue <= 0.8)
				return type_to_col(Savanna, noiseValue, x, y);
			if (tempValue <= 1)
				return type_to_col(Desert, noiseValue, x, y);
		}

		if (moistureValue <= 0.8)
		{
			if (tempValue <= 0.4)
				return type_to_col(Taiga, noiseValue, x, y);

			return type_to_col(TemperateForest, noiseValue, x, y);
		}

		if (moistureValue <= 1)
		{
			if (tempValue <= 0.4)
				return type_to_col(Taiga, noiseValue, x, y);
			if (tempValue <= 0.8)
				return type_to_col(TemperateRainForest, noiseValue, x, y);

			return type_to_col(TropicalRainForest, noiseValue, x, y);
		}
	}

	return Color(0, 0, 0);
}

Color Noisey::type_to_col(TerrainTypes TT, float noiseValue, int x, int y)
{
	//printf("\nconverting type");


	Color col;
	float normalnoise = normalize(noiseValue, -1, 1);
	switch (TT)
	{
	case Polar_Ice:
		biomeType[x][y] = Polar_Ice;
		col = terrainColours.get_pixel(15, 0);
		return Color(col.r * normalnoise, col.g * normalnoise, col.b  * normalnoise);
		break;
	case Tundra:
		biomeType[x][y] = Tundra;
		col = terrainColours.get_pixel(8, 0);
		return Color(col.r * normalnoise, col.g * normalnoise, col.b * normalnoise);
		break;
	case Taiga:
		biomeType[x][y] = Taiga;
		col = terrainColours.get_pixel(7, 0);
		return Color(col.r * normalnoise, col.g * normalnoise, col.b * normalnoise);
		break;
	case TemperateForest:
		biomeType[x][y] = TemperateForest;
		col = terrainColours.get_pixel(4, 0);
		return Color(col.r * normalnoise, col.g * normalnoise, col.b * normalnoise);
		break;
	case TemperateRainForest:
		biomeType[x][y] = TemperateRainForest;
		col = terrainColours.get_pixel(2, 0);
		return Color(col.r * normalnoise, col.g * normalnoise, col.b * normalnoise);
		break;
	case Chaparral:
		biomeType[x][y] = Chaparral;
		col = terrainColours.get_pixel(7, 0);
		return Color(col.r * normalnoise, col.g * normalnoise, col.b * normalnoise);
		break;
	case TropicalRainForest:
		biomeType[x][y] = TropicalRainForest;
		col = terrainColours.get_pixel(0, 0);
		return Color(col.r * normalnoise, col.g * normalnoise, col.b * normalnoise);
		break;
	case Savanna:
		biomeType[x][y] = Savanna;
		col = terrainColours.get_pixel(9, 0);
		return Color(col.r * normalnoise, col.g * normalnoise, col.b * normalnoise);
		break;
	case Desert:
		biomeType[x][y] = Desert;
		col = terrainColours.get_pixel(10, 0);
		return Color(col.r * normalnoise, col.g * normalnoise, col.b * normalnoise);
		break;
	case Ocean:
		biomeType[x][y] = Ocean;
		return Color(0 * normalnoise, 0.615 * normalnoise, 0.941 * normalnoise);
		break;
	case ShallowWater:
		biomeType[x][y] = ShallowWater;
		return Color(0 * normalnoise, 0.670 * normalnoise, 0.941 * normalnoise);
		break;
	case Beach:
		biomeType[x][y] = Beach;
		return Color(0.984 * normalnoise, 0.972 * normalnoise, 0.356 * normalnoise);
		break;
	default:
		break;
	}
}

Color normalizeRGB(Vector3 val) {

	float r = val.x, g = val.y, b = val.z;


	r = normalize(r, 0, 255);
	g = normalize(g, 0, 255);
	b = normalize(b, 0, 255);

	return Color(r, g, b);
}

float normalize(float val, int min, int max) {

	float normVal;
	
	normVal = (val - min) / (max - min);

	return normVal;

}