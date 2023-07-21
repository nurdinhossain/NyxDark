#pragma once 
#include <string>
#include <vector>
using namespace std;

// Particle class for particle swarm optimization
class Particle
{
    public:
        // constructor
        Particle();

        // destructor
        ~Particle();

        // initialize particle
        void initialize();

        // update particle
        void update_velocity(double inertia, double cognitive, double social, vector<float>& global_best_position);
        void update_position();

        // get position
        vector<float> getPosition();

        // get velocity
        vector<float> getVelocity();

        // get best position
        vector<float> getBestPosition();

        // get best mse
        double getBestMSE();

        // evaluate particle
        bool evaluate(vector<string>& lines, vector<float>& results, float k, bool& locked);

    private:
        // position
        vector<float> position;

        // velocity
        vector<float> velocity;

        // best position
        vector<float> best_position;

        // best mse
        double best_mse;
};

// function for evaluating multiple particles
void evaluateParticles(vector<Particle>& particles, int startIndex, int numParticles, vector<string>& lines, vector<float>& results, float k, bool& improved, vector<float>& global_best_position, double& global_best_mse, bool& locked);

// function for particle swarm optimization
void pso(string filename, float k, int num_particles, double inertia, double cognitive, double social, int numThreads);

// process file of FENs
void processFENs(string filename, vector<string>& lines, vector<float>& results);

// given a txt file of FENs + game result, open the file and get mean squared error of quiesce()
float mse(vector<string>& lines, vector<float>& results, float k);

// find the best k value for tuning
void findBestK(string filename, float start, float end, float step);

// vectorize parameters
vector<int*> vectorizeParameters();

// vectorize second half of first half of tables
vector<int*> vectorizeTablesSecondHalf();

// load in parameters from file
void loadParameters(string filename);

// get parameters from pointers
vector<int> getParametersFromPointers(vector<int*> pointers);

// texel tuning
void tune(string filename, float k);

// save parameters to file
void saveParameters(string filename, vector<int> parameters);
