#include "tuner.h"
#include "game.h"
#include "search.h"
#include "evalparams.h"
#include "tables.h"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <thread>
using namespace std;

// Particle class for particle swarm optimization
Particle::Particle()
{
    // initialize position and velocity
    initialize();
}

// destructor
Particle::~Particle()
{
    // nothing to do here
}

// initialize particle
void Particle::initialize()
{
    vector<int*> parameters = vectorizeParameters();
    
    // initialize position
    for (int i = 0; i < parameters.size(); i++)
    {
        // set position to parameter value
        position.push_back(*parameters[i]);

        // set best position to position
        best_position.push_back(position[i]);

        // get random float value between -2 and 2 (inclusive) for velocity
        float random = ((float)rand() / RAND_MAX) * 4 - 2;
        velocity.push_back(random);
    }

    // set best mse to 1000000
    best_mse = 1000000;
}

// update particle
void Particle::update_velocity(double inertia, double cognitive, double social, vector<float>& global_best_position)
{
    // get random values between 0 and 1
    double r1 = (double) rand() / RAND_MAX;
    double r2 = (double) rand() / RAND_MAX;

    // update velocity
    for (int i = 0; i < velocity.size(); i++)
    {
        velocity[i] = inertia * velocity[i] + cognitive * r1 * (best_position[i] - position[i]) + social * r2 * (global_best_position[i] - position[i]);
    }
}

void Particle::update_position()
{
    // update position
    for (int i = 0; i < position.size(); i++)
    {
        position[i] += velocity[i];
    }
}

// get position
vector<float> Particle::getPosition()
{
    return position;
}

// get velocity
vector<float> Particle::getVelocity()
{
    return velocity;
}

// get best position
vector<float> Particle::getBestPosition()
{
    return best_position;
}

// get best mse
double Particle::getBestMSE()
{
    return best_mse;
}

// evaluate particle
bool Particle::evaluate(vector<string>& lines, vector<float>& results, float k, bool& locked)
{
    vector<int> intPosition;
    for (int i = 0; i < position.size(); i++)
    {
        // round and convert to int
        intPosition.push_back(round(position[i]));
    }

    // save/load parameters only if unlocked
    while (locked)
    {
        // wait
    }
    locked = true;
    saveParameters("parameters.txt", intPosition);
    loadParameters("parameters.txt");
    locked = false;

    // get mse
    double score = mse(lines, results, k);

    // update best position and best mse
    if (score < best_mse)
    {
        best_position = position;
        std::cout << "Old best mse: " << best_mse << ", new best mse: " << score << std::endl;
        best_mse = score;

        return true;
    }

    return false;
}

// function for evaluating multiple particles
void evaluateParticles(vector<Particle>& particles, int startIndex, int numParticles, vector<string>& lines, vector<float>& results, float k, bool& improved, vector<float>& global_best_position, double& global_best_mse, bool& locked)
{
    // evaluate particles
    for (int i = startIndex; i < startIndex + numParticles; i++)
    {
        // evaluate particle
        bool particleImproved = particles[i].evaluate(lines, results, k, locked);

        // update improved
        if (particleImproved)
        {
            improved = true;
        }

        // get best position and mse
        vector<float> best_position = particles[i].getBestPosition();
        double best_mse = particles[i].getBestMSE();

        // update global best position and mse
        if (best_mse < global_best_mse)
        {
            global_best_position = best_position;
            std::cout << "Old global best mse: " << global_best_mse << ", new global best mse: " << best_mse << std::endl;
            global_best_mse = best_mse;
        }

        std::cout << "Particle " << i + 1 << " done" << std::endl;
    }
}

// function for particle swarm optimization
void pso(string filename, float k, int num_particles, double inertia, double cognitive, double social, int num_threads)
{
    // process file of FENs
    vector<string> lines;
    vector<float> results;
    processFENs(filename, lines, results);

    // initialize particles
    vector<Particle> particles;
    for (int i = 0; i < num_particles; i++)
    {
        Particle particle = Particle();
        particles.push_back(particle);
    }

    // initialize global best position and mse
    vector<float> global_best_position;
    double global_best_mse = 1000000;

    // lock for saving/loading parameters
    bool locked = false;

    // evaluate random particle to get best position and mse
    int randomIndex = rand() % num_particles;
    particles[randomIndex].evaluate(lines, results, k, locked);
    global_best_position = particles[randomIndex].getBestPosition();
    global_best_mse = particles[randomIndex].getBestMSE();
    
    std::cout << "Global best mse: " << global_best_mse << endl;

    // update all particles velocities and positions
    for (int i = 0; i < particles.size(); i++)
    {
        particles[i].update_velocity(inertia, cognitive, social, global_best_position);
        particles[i].update_position();
    }

    // run pso
    bool improved = true;
    int i = 0;
    while (improved)
    {
        // set improved to false
        improved = false;

        // initialize threads
        vector<thread> threads;

        // evaluate particles
        int particlesPerThread = num_particles / num_threads;
        for (int j = 0; j < num_threads; j++)
        {
            // create thread
            thread t(evaluateParticles, ref(particles), j * particlesPerThread, particlesPerThread, ref(lines), ref(results), k, ref(improved), ref(global_best_position), ref(global_best_mse), ref(locked));

            // add thread to vector
            threads.push_back(move(t));
        }

        // wait for threads to finish
        for (int j = 0; j < num_threads; j++)
        {
            threads[j].join();
        }

        // update particles
        for (int j = 0; j < particles.size(); j++)
        {
            // update velocity and position
            std::cout << "Updating particle " << j + 1 << std::endl;
            particles[j].update_velocity(inertia, cognitive, social, global_best_position);
            particles[j].update_position();
        }

        // print progress
        std::cout << "Iteration " << i + 1 << " complete" << endl;
        i++;

        // print global best mse
        std::cout << "Global best mse: " << global_best_mse << endl;

        // save best parameters
        vector<int> intPosition;
        for (int j = 0; j < global_best_position.size(); j++)
        {
            intPosition.push_back(round(global_best_position[j]));
        }

        saveParameters("best_parameters.txt", intPosition);
    }
}

// process file of FENs
void processFENs(string filename, vector<string>& lines, vector<float>& results)
{
    // open file and get lines
    ifstream file(filename);
    string line;
    while (getline(file, line))
    {
        // split line into fen, result by comma
        string delimiter = ",";
        size_t pos = 0;
        string token;
        while ((pos = line.find(delimiter)) != string::npos)
        {
            // get token
            token = line.substr(0, pos);
            lines.push_back(token);
            line.erase(0, pos + delimiter.length());
        }
        results.push_back(stof(line));

        // print progress
        if (lines.size() % 10000 == 0)
        {
            std::cout << lines.size() << " lines processed" << endl;
        }
    } 

    // close the file 
    file.close();
}

// given a txt file of FENs + game result, open the file and get mean squared error of quiesce()
float mse(vector<string>& lines, vector<float>& results, float k)
{
    // go through each line and get mse
    float total = 0;
    AI ai = AI();
    for (int i = 0; i < lines.size(); i++)
    {
        // get fen and result
        string fen = lines[i];
        float result = results[i];

        // get board
        Board board = Board(fen);

        // get quiesce score (relative to white)
        float score = ai.quiesce(board, NEG_INF, POS_INF) * (board.getNextMove() == WHITE ? 1 : -1);

        // map quiesce score to sigmoid using pow function
        float sigmoid = 1 / (1 + pow(10, (-k * score) / 400));

        // add to total
        total += pow(sigmoid - result, 2);
    }
    
    // return mse
    return total / lines.size();
}

// find the best k value for tuning
void findBestK(std::string filename, float start, float end, float step)
{
    // get lines and results
    vector<string> lines;
    vector<float> results;
    processFENs(filename, lines, results);

    // initialize best k and mse
    float bestK = start;
    float bestMSE = 1000000;

    // go through each k value
    for (float k = start; k <= end; k += step)
    {
        // get mse
        float m = mse(lines, results, k);

        // if mse is less than best mse, update best mse and best k
        if (m < bestMSE)
        {
            bestMSE = m;
            bestK = k;
            std::cout << "Best k: " << bestK << std::endl;
        }

        // print k and mse
        std::cout << "k: " << k << endl;
        std::cout << "MSE: " << m << endl;
    }

    // print best k and mse
    std::cout << "Best k: " << bestK << endl;
    std::cout << "Best MSE: " << bestMSE << endl;
}

// vectorize parameters
std::vector<int*> vectorizeParameters()
{
    // initialize vector of parameters
    vector<int*> parameters;

    // add parameters to vector
    for (int piece = 0; piece < 6; piece++)
    {
        for (int phase = 0; phase < 2; phase++)
        {
            for (int row = 0; row < 8; row++)
            {
                for (int col = 0; col < 4; col++)
                {
                    parameters.push_back(&TABLES[piece][phase][row * 8 + col]);
                }
            }
        }
    }

    parameters.push_back(&PASSED_PAWN);
    parameters.push_back(&UNOBSTRUCTED_PASSER);
    parameters.push_back(&UNSTOPPABLE_PASSED_PAWN);
    parameters.push_back(&CANDIDATE_PASSED_PAWN);
    parameters.push_back(&UNOBSTRUCTED_CANDIDATE);
    parameters.push_back(&BACKWARD_PAWN_PENALTY);
    parameters.push_back(&ISOLATED_PAWN_PENALTY);

    parameters.push_back(&KNIGHT_OUTPOST);
    parameters.push_back(&KNIGHT_OUTPOST_ON_HOLE);
    parameters.push_back(&KNIGHT_MOBILITY_MULTIPLIER);
    parameters.push_back(&KNIGHT_MOBILITY_OFFSET);

    parameters.push_back(&BISHOP_PAIR);
    parameters.push_back(&BISHOP_MOBILITY_MULTIPLIER);
    parameters.push_back(&BISHOP_MOBILITY_OFFSET);

    parameters.push_back(&ROOK_OPEN_FILE);
    parameters.push_back(&ROOK_HORIZONTAL_MOBILITY_MULTIPLIER);
    parameters.push_back(&ROOK_HORIZONTAL_MOBILITY_OFFSET);
    parameters.push_back(&ROOK_VERTICAL_MOBILITY_MULTIPLIER);
    parameters.push_back(&ROOK_VERTICAL_MOBILITY_OFFSET);
    parameters.push_back(&ROOK_CONNECTED);

    parameters.push_back(&KING_BLOCK_ROOK_PENALTY);
    parameters.push_back(&SAFETY_TABLE_MULTIPLIER);
    parameters.push_back(&MINOR_ATTACK_UNITS);
    parameters.push_back(&ROOK_ATTACK_UNITS);
    parameters.push_back(&QUEEN_ATTACK_UNITS);
    parameters.push_back(&ROOK_CHECK_UNITS);
    parameters.push_back(&QUEEN_CHECK_UNITS);
    parameters.push_back(&PAWN_SHIELD);
    parameters.push_back(&PAWN_STORM);
    parameters.push_back(&PAWN_SHIELD_DIVISOR);
    parameters.push_back(&PAWN_STORM_DIVISOR);

    return parameters;
}   

// vectorize second half of first half of tables
vector<int*> vectorizeTablesSecondHalf()
{
    // initialize vector of parameters
    vector<int*> parameters;

    // add parameters to vector
    for (int piece = 0; piece < 6; piece++)
    {
        for (int phase = 0; phase < 2; phase++)
        {
            for (int row = 0; row < 8; row++)
            {
                for (int col = 7; col >= 4; col--)
                {
                    parameters.push_back(&TABLES[piece][phase][row * 8 + col]);
                }
            }
        }
    }

    return parameters;
}

void loadParameters(string filename)
{
    ifstream file(filename);
    string line;
    int i = 0;
    vector<int*> parameters = vectorizeParameters();
    vector<int*> secondHalf = vectorizeTablesSecondHalf();

    // split comma-separated values
    while (getline(file, line))
    {
        string delimiter = ",";
        size_t pos = 0;
        string token;
        while ((pos = line.find(delimiter)) != string::npos)
        {
            // get token
            token = line.substr(0, pos);
            int param = stoi(token);
            *parameters[i] = param;

            // second half
            if (i < secondHalf.size())
                *secondHalf[i] = param;

            line.erase(0, pos + delimiter.length());
            i++;
        }

        // get last token
        int param = stoi(line);

        // second half
        if (i < secondHalf.size())
            *secondHalf[i] = param;

        *parameters[i] = param;
        i++;
    }

    file.close();
}

// get parameters from pointers
std::vector<int> getParametersFromPointers(vector<int*> pointers)
{
    vector<int> parameters;
    for (int i = 0; i < pointers.size(); i++)
    {
        parameters.push_back(*pointers[i]);
    }
    return parameters;
}

// texel tuning
void tune(string filename, float k)
{
    // get parameters
    vector<int*> parameters = vectorizeParameters();

    // get lines and results
    vector<string> lines;
    vector<float> results;
    processFENs(filename, lines, results);

    // initialize best mse
    float bestMSE = mse(lines, results, k);

    // improved flag
    bool improved = true;

    while (improved)
    {
        improved = false;

        // go through each parameter
        for (int i = 0; i < parameters.size(); i++)
        {
            // get parameter
            int* parameter = parameters[i];

            // get original parameter
            int originalParameter = *parameter;

            // increase parameter
            *parameter = originalParameter + 1;
            saveParameters("parameters.txt", getParametersFromPointers(parameters));
            loadParameters("parameters.txt");
            float increasedMSE = mse(lines, results, k);

            // decrease parameter
            *parameter = originalParameter - 1;
            saveParameters("parameters.txt", getParametersFromPointers(parameters));
            loadParameters("parameters.txt");
            float decreasedMSE = mse(lines, results, k);

            // reset parameter
            *parameter = originalParameter;
            saveParameters("parameters.txt", getParametersFromPointers(parameters));
            loadParameters("parameters.txt");

            // update parameter if improved
            if (increasedMSE < bestMSE && decreasedMSE < bestMSE)
            {
                if (increasedMSE < decreasedMSE)
                {
                    *parameter = originalParameter + 1;
                }
                else
                {
                    *parameter = originalParameter - 1;
                }
                improved = true;
                saveParameters("parameters.txt", getParametersFromPointers(parameters));
            }
            else if (increasedMSE < bestMSE)
            {
                *parameter = originalParameter + 1;
                improved = true;
                saveParameters("parameters.txt", getParametersFromPointers(parameters));
            }
            else if (decreasedMSE < bestMSE)
            {
                *parameter = originalParameter - 1;
                improved = true;
                saveParameters("parameters.txt", getParametersFromPointers(parameters));
            }

            // update best mse
            bestMSE = min(bestMSE, min(increasedMSE, decreasedMSE));

            // load parameters
            loadParameters("parameters.txt");

            // print parameter and mses
            std::cout << "Parameter: " << i << endl;
            std::cout << "Original MSE: " << bestMSE << endl;
            std::cout << "Increased MSE: " << increasedMSE << endl;
            std::cout << "Decreased MSE: " << decreasedMSE << endl;
        }

        // save a stable version of the parameters
        saveParameters("stable_parameters.txt", getParametersFromPointers(parameters));
    }
}

// save parameters to file
void saveParameters(string filename, vector<int> parameters)
{
    ofstream outputFile;
    outputFile.open(filename);
    for (int i = 0; i < parameters.size(); i++)
    {
        outputFile << parameters[i];
        if (i != parameters.size() - 1)
        {
            outputFile << ",";
        }
    }
    outputFile.close();
}