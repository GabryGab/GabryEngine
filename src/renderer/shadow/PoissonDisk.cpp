#include "PoissonDisk.h"
#include <cmath>
#include <iostream>
#include <random>
#define _USE_MATH_DEFINES
#include <math.h>
#include <list>

#define PI 3.14

// Function to determine whether candidate is valid or not.
bool isValid(const glm::vec2& candidate, const glm::vec2& sampleRegionSize,
	float cellSize, float radius, const std::vector<glm::vec2>& points, int** grid, int nRows, int nCols);


// Only generates at most ~20 points on average.
// If we want to make the function work with every value, separate the creation of the poisson disk in another function
// and call it again with a lower radius to get more values until we have enough points to work with.
std::vector<glm::vec2> PoissonDisk::generatePoissonDisk(int pcfSamples) {
	// sampleRegionSize is 1x1 for simplicity.
	// "Range" is modified later with another value as not to generate another poisson disk.
	// If changed probably breaks everything.
	const glm::vec2 sampleRegionSize = glm::vec2(1.0f, 1.0f);

	// Radius of our circle.
	// Circles will not be able to intersect with each other.
	float radius = 0.15f;

	float cellSize = radius / std::sqrt(2);

	// Remember to delete arrays after use.
	// Note that rows and cols are always the same number.
	int nRows = int(std::ceil(sampleRegionSize.x / cellSize)), nCols = int(std::ceil(sampleRegionSize.x / cellSize));
	int** grid = new int* [nRows];
	for (int i = 0; i < nRows; ++i) {
		grid[i] = new int[nCols];
	}
	// Initialize all values to 0.
	// In the code that creates the actual values of the points an index of 0 will mean not created.
	for (int i = 0; i < nRows; ++i) {
		for (int j = 0; j < nCols; ++j) {
			grid[i][j] = 0;
		}
	}

	std::vector<glm::vec2> points;
	std::vector<glm::vec2> spawnPoints;

	std::random_device rd; // obtain a random number from hardware
	std::mt19937 gen(rd()); // seed the generator

	std::srand(std::time(0));

	// Add central point.
	// Used later for normalizing poisson disk into a circle.
	// This is important, we want a central point for shadowing.
	glm::vec2 centralPoint(sampleRegionSize.x / 2, sampleRegionSize.y / 2);
	// Starting point can be the center but if so you can see the pixel edge of the shadow.
	glm::vec2 startingPoint(((float)std::rand()) / ((float)RAND_MAX), ((float)std::rand()) / ((float)RAND_MAX));
	points.push_back(startingPoint);
	grid[int(std::floor(startingPoint.x / cellSize))][int(std::floor(startingPoint.y / cellSize))] = static_cast<int>(points.size());

	spawnPoints.push_back(glm::vec2(sampleRegionSize.x / 2, sampleRegionSize.y / 2));

	while (spawnPoints.size() > 0) {
		// Create random number generator in range(0,spawnPoints.size()).
		std::uniform_int_distribution<> distr(0, spawnPoints.size() - 1); // define the range

		int spawnIndex = distr(gen);
		glm::vec2 spawnCentre = spawnPoints[spawnIndex];

		// 30 is a good values but can be changed.
		int numSamplesBeforeRejection = 30;

		bool candidateAccepted = false;
		for (int i = 0; i < numSamplesBeforeRejection; ++i) {
			float angle = ((float)std::rand()) / ((float)RAND_MAX) * M_PI * 2;
			glm::vec2 direction(std::sin(angle), std::cos(angle));
			glm::vec2 candidate = spawnCentre + direction * (((float)std::rand()) / ((float)RAND_MAX) * radius + radius);
			if (isValid(candidate, sampleRegionSize, cellSize, radius, points, grid, nRows, nCols)) {
				points.push_back(candidate);
				spawnPoints.push_back(candidate);
				grid[int(std::floor(candidate.x / cellSize))][int(std::floor(candidate.y / cellSize))] = static_cast<int>(points.size());
				candidateAccepted = true;
				break;
			}
		}

		if (!candidateAccepted) {
			spawnPoints.erase(spawnPoints.begin() + spawnIndex);
		}
	}

	// Delete grid array.
	for (int i = 0; i < nRows; ++i)
		delete[] grid[i];
	delete[] grid;

	/*for (auto x : points) {
		std::cout << x.x << " " << x.y << std::endl;
	}
	std::cout << std::endl << points.size() << std::endl;*/

	struct LengthIndex {
		int index;
		float length;
		bool operator<(const LengthIndex& li) {
			return length < li.length;
		}
	};

	// First pass.
	// Filter all points inside the circle of maximum radius for our grid (radius = 0.5f for grid 1x1) 
	// and save their lengths (used to get the first x number of points).
	float acceptanceRadius = sampleRegionSize.x * 0.5f; // NOT to mistake with diameter.
	std::vector<glm::vec2> circleFilteredPoints;
	std::vector<LengthIndex> lengthIndexList;
	for (const auto& point : points) {
		glm::vec2 circlePoint = point - centralPoint;
		float length = glm::length(circlePoint);
		if (length < acceptanceRadius) {
			LengthIndex li;
			li.index = circleFilteredPoints.size();
			li.length = length;
			lengthIndexList.push_back(li);
			circleFilteredPoints.push_back(circlePoint);
		}
	}
	// Sort list so that we can get the first x number of entries to build our final points.
	std::sort(lengthIndexList.begin(), lengthIndexList.end());

	// Second pass.
	// Actually calculate final points based on how many we want.
	// The results are filtered to go from -1 to 1 (obviously in a circle manner).
	// 16 is maximum number of points allowed.
	int numberPointsNeeded = pcfSamples <= 16? pcfSamples : 16; // This values can be changed to affect quality but also performance. 			
	if (circleFilteredPoints.size() < numberPointsNeeded)
		numberPointsNeeded = circleFilteredPoints.size(); // Change number of points needed to a lower value if we don't have enough.
	std::vector<glm::vec2> finalPoints;
	float scaleValue = 1.0f / lengthIndexList[numberPointsNeeded - 1].length;
	for (int i = 0; i < numberPointsNeeded; ++i) {
		finalPoints.push_back(circleFilteredPoints[lengthIndexList[i].index] * scaleValue);
	}

	/*for (auto x : finalPoints) {
		std::cout << x.x << " " << x.y << std::endl;
	}
	std::cout << std::endl;
	for (auto x : finalPoints) {
		std::cout << glm::length(x) << std::endl;
	}
	std::cout << std::endl << finalPoints.size() << std::endl;*/

	return finalPoints;
}

bool isValid(const glm::vec2& candidate, const glm::vec2& sampleRegionSize,
	float cellSize, float radius, const std::vector<glm::vec2>& points, int** grid, int nRows, int nCols) {

	if (candidate.x >= 0 && candidate.x < sampleRegionSize.x && candidate.y >= 0 && candidate.y < sampleRegionSize.y) {
		int cellX = int(candidate.x / cellSize);
		int cellY = int(candidate.y / cellSize);

		int searchStartX = std::max(0, cellX - 2);
		int searchEndX = std::min(cellX + 2, nRows - 1);
		int searchStartY = std::max(0, cellY - 2);
		int searchEndY = std::min(cellY + 2, nCols - 1);

		for (int x = searchStartX; x <= searchEndX; ++x) {
			for (int y = searchStartY; y <= searchEndY; ++y) {
				int pointIndex = grid[x][y] - 1;
				if (pointIndex != -1) {
					float dst = glm::length((candidate - points[pointIndex]));
					if (dst < radius) {
						return false;
					}
				}
			}
		}

		return true;
	}
	return false;
}