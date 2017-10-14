
#ifndef GENMAP_H
#define GENMAP_H

#include "reference.h"
#include "../scene/2d/tile_map.h"

enum TerrainTypes // rainfall, temp
{
	//Low TEMP
	Polar_Ice, //verylow, verylow
	Tundra, //low, low
	Taiga, //low - moderate, low
	TemperateForest, //moderate, moderate
	TemperateRainForest, //high, moderate
	Chaparral, // low, moderate
	TropicalRainForest, //high, high
	Savanna, //low , high
	Desert, //veryLow , high
	//High TEMP

	Ocean,
	ShallowWater,
	Beach
};

enum River {
	Source,
	Water,
	None
};

class Noisey : public Reference {
	OBJ_TYPE(Noisey, Reference);

public:
	float noise[10000][10000];
	float moisture[10000][10000];
	float temperature[10000][10000];
	float frequency, exponent;
	int mapHeight;
	int mapWidth;
	River river[10000][10000];
	TerrainTypes biomeType[10000][10000];
	Image tileImage;
	Image map;
	Image terrainColours;
	Image moistureMap;
	Image elevationMap;
	Image tempMap;
	Image riverMap;

protected:
	static void _bind_methods();

public:
	void set_exponent(float val);
	void set_frequency(float val);
	void set_image(Image img);
	void create_map(int mapsizex, int mapsizey);
	Image get_map();
	Image get_elevationMap();
	Image get_moistureMap();
	Image get_heatMap();
	Image get_riverMap();
	void resetRiver();
	Noisey();

private:
	void createNoise();
	void createRivers();
	void applyRiverMoisture();
	void paintMap();
	float constrainNoise(float noiseValue);
	Color type_to_col(TerrainTypes TT, float noiseVal, int x, int y);
	Color get_col(float noiseValue, float moistureValue, float tempValue, int x, int y);
	bool nearbySourceCheck(Vector2 xy, int x, int y);
};

#endif

