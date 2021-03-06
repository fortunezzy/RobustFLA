//
// Created by chenghaowang on 14/07/17.
//

#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <Python.h>
#include <cmath>
#include <vector>
#include <set>
#include <cstdio>
#include <map>
#include <random>
#include <boost/math/special_functions.hpp>
#include <queue>

#ifndef K_CONST
#define K_CONST
const double K = 1.8520108;
#endif
#ifndef EARTH_RADIUS_CONST
#define EARTH_RADIUS_CONST
const double EARTH_RADIUS = 6378.16;
#endif
#ifndef MIN_SEPARATION_DISTANCE_CONST
#define MIN_SEPARATION_DISTANCE_CONST
const double MIN_SEPARATION_DISTANCE = 5.0;
#endif
#ifndef MIN_ANGLE_CONST
#define MIN_ANGLE_CONST
const double MIN_ANGLE = cos(5.0 / 180.0 * M_PI);
#endif
#ifndef MIN_PROBA_CONST
#define MIN_PROBA_CONST
/**
* A coefficient parameter: MIN_PORBA, the minimal considering conflict probability.
*/
const double MIN_PROBA = 0.00001;
#endif
#ifndef MIN_SIGMA_CONST
#define MIN_SIGMA_CONST
const double MIN_SIGMA = 0.1;
#endif
typedef std::vector<int> viList;
typedef std::vector<std::vector<int>> vviList;
typedef std::vector<double> vdList;
typedef std::deque<std::vector<int>> qviList;
typedef int Level;
typedef std::set<Level> LevelSet;
typedef std::vector<Level> LevelVector;
typedef std::map<Level, std::pair<int, int>> FlightLevelMap;
typedef std::map<Level, bool> LevelExamine;
typedef std::vector<std::pair<Level, std::pair<int, int>>> LevelNumberList;
typedef std::string String;
typedef double Time;
typedef std::normal_distribution<double> normal_dist;
typedef std::uniform_real_distribution<double> uni_dist;

////////////Definition of Flight Level/////////////////////////////////////////
#ifndef LEVEL_LIST_CONST
#define LEVEL_LIST_CONST
#ifdef NRVSM
static viList const LevelIFRA{10, 30, 50, 70, 90, 110, 130, 150, 170, 190, 210, 230, 250, 270, 290, 330, 370, 410, 450, 490};
static viList const LevelIFRB{20, 40, 60, 80, 100, 120, 140, 160, 180, 200, 220, 240, 260, 280, 310, 350, 390, 430, 470, 510};
#else
const viList LevelIFRA{10, 30, 50, 70, 90, 110, 130, 150, 170, 190, 210, 230, 250, 270, 290, 310, 330, 350, 370, 390,
                       410, 450, 490};
const viList LevelIFRB{20, 40, 60, 80, 100, 120, 140, 160, 180, 200, 220, 240, 260, 280, 300, 320, 340, 360, 380,
                       400, 430, 470, 510};
#endif
const viList LevelVFRA{35, 55, 75, 95, 115, 135, 155, 175, 195};
const viList LevelVFRB{45, 65, 85, 105, 125, 145, 165, 185};
#endif
////////////Definition of Flight Level/////////////////////////////////////////

/**
 * A coefficient parameter: A_BAR, from article "A Geometrical Approach to Conflict Probability Estimation, Richard Irvine"
 */
double getA_Bar();

////////////Method template////////////////////////////////////////////////////
/**
 * A template of the method to verify whether the a vector contains an element.
 * @tparam T                    the general type of data
 * @param vObjectsVector        the target vector
 * @param object                the queried object
 * @return true, if the vector contains the queried object; false, otherwise.
 */
template<typename T>
bool contains(std::vector<T> vObjectsVector, T object) {
    if (vObjectsVector.size() < 1) return false;
    for (auto &&item : vObjectsVector) {
        if (item == object) {
            return true;
        }
    }
    return false;
}

/**
 * A template of the method to verify whether the a vector contains an element, where the element is a pointer.
 * @tparam T                    the general type of data
 * @param vpObjectsVector       the target vector
 * @param pObject               the queried object
 * @return true, if the vector contains the queried object; false, otherwise.
 */
template<typename T>
bool contains(std::vector<T *> vpObjectsVector, T *pObject) {
    if (vpObjectsVector.size() < 1) return false;
    for (auto &&item : vpObjectsVector) {
        if (*item == *pObject) {
            return true;
        }
    }
    return false;
}

/**
 * A template of the method to find an object by providing its code.
 * @tparam T                    The general type of object
 * @param vpObjectsVector       The object list
 * @param sCode                 The given code of an object
 * @return The pointer of corresponding object.
 */
template<typename T>
T *findByCode(std::vector<T *> vpObjectsVector, const String &sCode) {
    if (vpObjectsVector.size() < 1) return nullptr;
    for (auto &&item : vpObjectsVector) {
        if (item->getCode() == sCode) {
            return item;
        }
    }
    return nullptr;
}


/**
 * A template of Comparator for a pair object in a vector.
 * @tparam T1   The type of key in a pair object
 * @tparam T2   The type of value in a pair object
 */
template<typename T1, typename T2, typename T3>
struct greater_second {
    typedef std::pair<T1, std::pair<T2, T3>> type;

    bool operator()(type const &a, type const &b) const {
        return (a.second.second > b.second.second) ||
               (a.second.second == b.second.second && a.second.first > b.second.first);
    }
};


/**
 * A template to retrieve key from a pair object in a vector.
 * @tparam T1   The type of key in a pair object
 * @tparam T2   The type of value in a pair object
 */
template<typename T1, typename T2, typename T3>
struct retrieve_key {
    typedef std::pair<T1, std::pair<T2, T3>> type;

    T1 operator()(type const &a) const {
        return a.first;
    }
};

////////////Method template////////////////////////////////////////////////////
double getSigma1(double alpha, double beta, double gamma);

double getSigma2(double alpha, double beta, double gamma);

double getSigma3(double alpha, double beta, double gamma, double w1, double w2, double p);

/**
     * Find the feasible flight level list.
     * @param iDefaultLevel     A given default flight level
     * @return A three elements list.
     */
viList findFeasibleLevels(int iDefaultLevel, int iSize);

Level findNextFeasibleLevel(int iDefaultLevel, Level lastLevel);
#endif //CONFIGURATION_H
