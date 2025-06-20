/*
  ==============================================================================

    Pattern.cpp
    Author:  tiagolr

  ==============================================================================
*/

#include "Pattern.h"
#include <cmath>
#include <algorithm>
#include "../PluginProcessor.h"

std::vector<PPoint> Pattern::copy_pattern;

Pattern::Pattern(int i)
{
    index = i;
    incrementVersion();
}

void Pattern::incrementVersion()
{
    versionID = versionIDCounter;
    versionIDCounter += 1;
}

void Pattern::sortPoints()
{
    std::sort(points.begin(), points.end(), [](const PPoint& a, const PPoint& b) {
        return a.x < b.x;
    });
}

void Pattern::sortPointsSafe()
{
    std::lock_guard<std::mutex> lock(pointsmtx);
    clearTransform();
    sortPoints();
}

void Pattern::setTension(double t, double tatk, double trel, bool dual)
{
    dualTension = dual;
    tensionAtk.store(tatk);
    tensionRel.store(trel);
    tensionMult.store(t);
}

int Pattern::insertPoint(double x, double y, double tension, int type, bool sort, bool clearsTails)
{
    std::lock_guard<std::mutex> lock(pointsmtx);
    auto id = pointsIDCounter;
    pointsIDCounter += 1;

    const PPoint p = { id, x, y, tension, type, clearsTails };
    points.push_back(p);
    if (sort)
        sortPoints();
    clearTransform();

    // return point index
    auto pidx = std::find_if(points.begin(), points.end(), [id](const PPoint& p) { return p.id == id; });
    return pidx == points.end() ? -1 : (int)std::distance(points.begin(), pidx);
};

int Pattern::insertPointUnsafe(double x, double y, double tension, int type, bool sort, bool clearsTails)
{
    auto id = pointsIDCounter;
    pointsIDCounter += 1;

    const PPoint p = { id, x, y, tension, type, clearsTails };
    points.push_back(p);
    if (sort)
        sortPoints();
    clearTransform();

    auto pidx = std::find_if(points.begin(), points.end(), [id](const PPoint& p) { return p.id == id; });
    return pidx == points.end() ? -1 : (int)std::distance(points.begin(), pidx);
};

void Pattern::removePoint(double x, double y)
{
    std::lock_guard<std::mutex> lock(pointsmtx);
    clearTransform();
    for (size_t i = 0; i < points.size(); ++i) {
        if (points[i].x == x && points[i].y == y) {
            points.erase(points.begin() + i);
            return;
        }
    }
}

void Pattern::removePoint(int i) {
    std::lock_guard<std::mutex> lock(pointsmtx);
    clearTransform();
    points.erase(points.begin() + i);
}

void Pattern::removePointsInRange(double x1, double x2)
{
    std::lock_guard<std::mutex> lock(pointsmtx);
    clearTransform();
    removePointsInRangeAux(x1, x2);
}

void Pattern::removePointsInRangeAux(double x1, double x2)
{
    for (auto i = points.begin(); i != points.end(); ++i) {
        if (i->x >= x1 && i->x <= x2) {
            i = points.erase(i);
            removePointsInRangeAux(x1, x2);
            return;
        }
    }
}

void Pattern::invert()
{
    std::lock_guard<std::mutex> lock(pointsmtx);
    clearTransform();
    for (auto i = points.begin(); i != points.end(); ++i) {
        i->y = 1 - i->y;
    }
    incrementVersion();
};

void Pattern::reverse()
{
    std::lock_guard<std::mutex> lock(pointsmtx);
    clearTransform();
    std::reverse(points.begin(), points.end());

    double t0 = !points.empty() ? points[0].tension : 0.0;
    int type0 = !points.empty() ? points[0].type : 1;
    for (size_t i = 0; i < points.size(); ++i) {
        auto& p = points[i];
        p.x = 1 - p.x;
        if (i < points.size() - 1) {
            p.tension = points[i + 1].tension * -1;
            p.type = points[i + 1].type;
        }
        else {
            p.tension = t0 * -1;
            p.type = type0;
        }
    }
    incrementVersion();
};

void Pattern::doublePattern()
{
    std::lock_guard<std::mutex> lock(pointsmtx);
    clearTransform();

    auto pts = points;
    for (auto& p : pts) {
        insertPointUnsafe(p.x + 1.0, p.y, p.tension, p.type, false);
    }

    for (auto& p : points) {
        p.x /= 2.0;
    }

    incrementVersion();
}

void Pattern::rotate(double x) {
    std::lock_guard<std::mutex> lock(pointsmtx);
    clearTransform();
    if (x > 1.0) x = 1.0;
    if (x < -1.0) x = -1.0;
    for (auto p = points.begin(); p != points.end(); ++p) {
        if (p->x == 0.0) p->x += 1e-9; // FIX - distinguish 1.0 and 0.0 points
        if (p->x == 1.0) p->x -= 1e-9; //
        p->x += x;
        if (p->x < 0.0) p->x += 1.0;
        if (p->x > 1.0) p->x -= 1.0;
    }
    sortPoints();
    incrementVersion();
}

void Pattern::rotateUnsafe(double x) {
    clearTransform();
    if (x > 1.0) x = 1.0;
    if (x < -1.0) x = -1.0;
    for (auto p = points.begin(); p != points.end(); ++p) {
        if (p->x == 0.0) p->x += 1e-9; // FIX - distinguish 1.0 and 0.0 points
        if (p->x == 1.0) p->x -= 1e-9; //
        p->x += x;
        if (p->x < 0.0) p->x += 1.0;
        if (p->x > 1.0) p->x -= 1.0;
    }
    sortPoints();
    incrementVersion();
}

void Pattern::clear()
{
    std::lock_guard<std::mutex> lock(pointsmtx);
    clearY = 1.0 - getavgY();
    clearTransform();
    points.clear();
    incrementVersion();
}

void Pattern::clearUnsafe()
{
    clearTransform();
    points.clear();
    incrementVersion();
}

void Pattern::buildSegments()
{
    std::vector<PPoint> pts;
    {
        std::lock_guard<std::mutex> lock(pointsmtx);
        pts = points;
    }
    // add ghost points outside the 0..1 boundary
    // allows the pattern to repeat itself and rotate seamlessly
    if (pts.size() == 0) {
        pts.push_back({0, -1.0, clearY, 0.0, 1, false});
        pts.push_back({0, 2.0, clearY, 0.0, 1, false});
    }
    else if (pts.size() == 1) {
        pts.insert(pts.begin(), {0, -1.0, pts[0].y, 0.0, 1, false});
        pts.push_back({0, 2.0, pts[0].y, 0.0, 1, false});
    }
    else {
        auto p1 = pts[0];
        auto p2 = pts[pts.size()-1];
        pts.insert(pts.begin(), {0, p2.x - 1.0, p2.y, p2.tension, p2.type, false});
        pts.push_back({0, p1.x + 1.0, p1.y, p1.tension, p1.type, false});
    }

    std::lock_guard<std::mutex> lock(mtx); // prevents crash while reading Y from another thread
    segments.clear();
    for (size_t i = 0; i < pts.size() - 1; ++i) {
        auto p1 = pts[i];
        auto p2 = pts[i + 1];
        segments.push_back({ p1.x, p2.x, p1.y, p2.y, p1.tension, 0, p1.type, p1.clearsTails });
    }

    // transfer segments clearTails to next segment if the segment length is very short
    // fixes segments not clearing tails when segment points are on the same xpos
    for (int i = 0; i < segments.size(); ++i) {
        auto& seg = segments[i];
        if (seg.clearsTails) {
            double length = seg.x2 - seg.x1;
            if (length < 1e-5) {
                auto& next = i < segments.size() - 1 ? segments[i + 1] : segments[0];
                next.clearsTails = true;
            }
        }
    }
}

// Thread safely returns a copy of segments
std::vector<Segment> Pattern::getSegments()
{
    std::lock_guard<std::mutex> lock(mtx);
    return segments;
}

void Pattern::loadSine() {
    std::lock_guard<std::mutex> lock(pointsmtx);
    clearUnsafe();
    insertPointUnsafe(0, 1, 0.2, 2);
    insertPointUnsafe(0.5, 0, 0.2, 2);
    rotateUnsafe(0.25);
}

void Pattern::loadTriangle() {
    std::lock_guard<std::mutex> lock(pointsmtx);
    clearUnsafe();
    insertPointUnsafe(0, 1, 0, 1);
    insertPointUnsafe(0.5, 0, 0, 1);
    insertPointUnsafe(1, 1, 0, 1);
};

void Pattern::loadRandom(int grid) {
    std::lock_guard<std::mutex> lock(pointsmtx);
    clearUnsafe();
    auto y = static_cast<double>(rand())/RAND_MAX;
    insertPointUnsafe(0, y, 0, 1);
    insertPointUnsafe(1, y, 0, 1);
    for (auto i = 0; i < grid; ++i) {
        auto r1 = static_cast<double>(rand()) / RAND_MAX;
        auto r2 = static_cast<double>(rand()) / RAND_MAX;
        insertPointUnsafe(std::min(0.9999999, std::max(0.000001, r1 / (double)grid + i / (double)grid)), r2, 0, 1);
    }
};

void Pattern::copy()
{
  copy_pattern = points;
}

void Pattern::paste()
{
  std::lock_guard<std::mutex> lock(pointsmtx);
  if (copy_pattern.size() > 0) {
    points = copy_pattern;
    incrementVersion();
  }
}

void Pattern::transform(double midy)
{
    {
        std::lock_guard<std::mutex> lock(pointsmtx);
        midy = 1.0 - midy; // y coordinates are inverted
        clearY = midy;

        if (rawpoints.empty()) 
            rawpoints = points;
        if (!rawpoints.empty()) {
            double avg = 0.0;
            for (auto& p : rawpoints) {
                avg += p.y;
            }
            avg /= points.size();

            if (avg <= midy) {
                if (avg == 1.0) avg -= 1e-10;
                double alpha = (midy - avg) / (1.0 - avg);
                for (int i = 0; i < points.size(); i++) {
                    points[i].y = rawpoints[i].y + alpha * (1 - rawpoints[i].y);
                }
            }
            else {
                if (avg == 0.0) avg += 1e-10;
                double beta = (avg - midy) / avg;
                for (int i = 0; i < points.size(); i++) {
                    points[i].y = (1.0 - beta) * rawpoints[i].y;
                }
            }
        }
    }

    incrementVersion();
    buildSegments();
}

void Pattern::clearTransform()
{
    rawpoints.clear();
}

double Pattern::getavgY()
{
    double avg = 0.0;
    auto pts = points;
    if (!pts.size()) return 1.0 - clearY;
    for (auto& p : pts) {
        avg += 1.0 - p.y;
    }
    avg /= pts.size();
    return avg;
}

/*
  Based of https://github.com/KottV/SimpleSide/blob/main/Source/types/SSCurve.cpp
*/
double Pattern::get_y_curve(Segment seg, double x)
{
    auto rise = seg.y1 > seg.y2;
    auto tmult = dualTension ? (rise ? tensionAtk.load() : tensionRel.load()) : tensionMult.load();
    auto ten = seg.tension + (rise ? -tmult : tmult);
    if (ten > 1) ten = 1;
    if (ten < -1) ten = -1;
    auto pwr = pow(1.1, std::fabs(ten * 50));

    if (seg.x1 == seg.x2)
        return seg.y2;

    if (ten >= 0)
        return std::pow((x - seg.x1) / (seg.x2 - seg.x1), pwr) * (seg.y2 - seg.y1) + seg.y1;

    return -1 * (std::pow(1 - (x - seg.x1) / (seg.x2 - seg.x1), pwr) - 1) * (seg.y2 - seg.y1) + seg.y1;
}

int Pattern::getWaveCount(Segment seg)
{
    if (seg.type == PointType::Pulse) return (int)(std::max(std::floor(std::pow(seg.tension,2) * 100), 1.0));
    if (seg.type == PointType::Wave) return (int)(std::floor(std::fabs(std::pow(seg.tension,2) * 100) + 1) - 1);
    if (seg.type == PointType::Triangle) return (int)(std::floor(std::fabs(std::pow(seg.tension,2) * 100) + 1) - 1.0);
    if (seg.type == PointType::Stairs) return (int)(std::max(std::floor(std::pow(seg.tension,2) * 150), 2.));
    if (seg.type == PointType::SmoothSt) return (int)(std::max(floor(std::pow(seg.tension,2) * 150), 1.0));
    return 0;
}

double Pattern::get_y_scurve(Segment seg, double x)
{
  auto rise = seg.y1 > seg.y2;
  auto tmult = dualTension ? (rise ? tensionAtk.load() : tensionRel.load()) : tensionMult.load();
  auto ten = seg.tension + (rise ? -tmult : tmult);
  if (ten > 1) ten = 1;
  if (ten < -1) ten = -1;
  auto pwr = pow(1.1, std::fabs(ten * 50));

  double xx = (seg.x2 + seg.x1) / 2;
  double yy = (seg.y2 + seg.y1) / 2;

  if (seg.x1 == seg.x2)
    return seg.y2;

  if (x < xx && ten >=0)
    return std::pow((x - seg.x1) / (xx - seg.x1), pwr) * (yy - seg.y1) + seg.y1;

  if (x < xx && ten < 0)
    return -1 * (std::pow(1 - (x - seg.x1) / (xx - seg.x1), pwr) - 1) * (yy - seg.y1) + seg.y1;

  if (x >= xx && ten >= 0)
    return -1 * (std::pow(1 - (x - xx) / (seg.x2 - xx), pwr) - 1) * (seg.y2 - yy) + yy;

   return std::pow((x - xx) / (seg.x2 - xx), pwr) * (seg.y2 - yy) + yy;
}

double Pattern::get_y_pulse(Segment seg, double x)
{
  double t = std::max(std::floor(std::pow(seg.tension,2) * 100), 1.0); // num waves

  if (x == seg.x2)
    return seg.y2;

  double cycle_width = (seg.x2 - seg.x1) / t;
  double x_in_cycle = cycle_width == 0.0 ? 0.0 : std::fmod((x - seg.x1), cycle_width);
  return x_in_cycle < cycle_width / 2
    ? (seg.tension >= 0 ? seg.y1 : seg.y2)
    : (seg.tension >= 0 ? seg.y2 : seg.y1);
}

double Pattern::get_y_wave(Segment seg, double x)
{
  double t = 2 * std::floor(std::fabs(std::pow(seg.tension,2) * 100) + 1) - 1; // wave num
  double amp = (seg.y2 - seg.y1) / 2;
  double vshift = seg.y1 + amp;
  double freq = t * 2 * PI / (2 * (seg.x2 - seg.x1));
  return -amp * cos(freq * (x - seg.x1)) + vshift;
}

double Pattern::get_y_triangle(Segment seg, double x)
{
  double tt = 2 * std::floor(std::fabs(std::pow(seg.tension,2) * 100) + 1) - 1.0;// wave num
  double amp = seg.y2 - seg.y1;
  double t = (seg.x2 - seg.x1) * 2 / tt;
  return amp * (2 * std::fabs((x - seg.x1) / t - std::floor(1./2. + (x - seg.x1) / t))) + seg.y1;
}

double Pattern::get_y_stairs(Segment seg, double x)
{
  double t = std::max(std::floor(std::pow(seg.tension,2) * 150), 2.); // num waves
  double step_size = 0.;
  double step_index = 0.;
  double y_step_size = 0.;

  if (seg.tension >= 0) {
    step_size = (seg.x2 - seg.x1) / t;
    step_index = std::floor((x - seg.x1) / step_size);
    y_step_size = (seg.y2 - seg.y1) / (t-1);
  }
  else {
    step_size = (seg.x2 - seg.x1) / (t-1);
    step_index = ceil((x - seg.x1) / step_size);
    y_step_size = (seg.y2 - seg.y1) / t;
  }

  if (x == seg.x2)
    return seg.y2;

  return seg.y1 + step_index * y_step_size;
}

double Pattern::get_y_smooth_stairs(Segment seg, double x)
{
  double pwr = 4;
  double t = std::max(floor(std::pow(seg.tension,2) * 150), 1.0); // num waves

  double gx = (seg.x2 - seg.x1) / t; // gridx
  double gy = (seg.y2 - seg.y1) / t; // gridy
  double step_index = std::floor((x - seg.x1) / gx);

  double xx1 = seg.x1 + gx * step_index;
  double xx2 = seg.x1 + gx * (step_index + 1);
  double xx = (xx1 + xx2) / 2;

  double yy1 = seg.y1 + gy * step_index;
  double yy2 = seg.y1 + gy * (step_index + 1);
  double yy = (yy1 + yy2) / 2;

  if (seg.x1 == seg.x2)
    return seg.y2;

  if (x < xx && seg.tension >= 0)
    return std::pow((x - xx1) / (xx - xx1), pwr) * (yy - yy1) + yy1;

  if (x < xx && seg.tension < 0)
    return -1 * (std::pow(1 - (x - xx1) / (xx - xx1), pwr) - 1) * (yy - yy1) + yy1;

  if (x >= xx && seg.tension >= 0)
    return -1 * (std::pow(1 - (x - xx) / (xx2 - xx), pwr) - 1) * (yy2 - yy) + yy;

  return std::pow((x - xx) / (xx2 - xx), pwr) * (yy2 - yy) + yy;
}


double Pattern::get_y_at(double x, bool updateClearTails)
{
    std::lock_guard<std::mutex> lock(mtx); // prevents crash while building segments
    int low = 0;
    int high = static_cast<int>(segments.size()) - 1;

    if (updateClearTails)
        shouldClearTails = false;

    // binary search the segment containing x
    while (low <= high) {
        int mid = (low + high) / 2;
        const auto& seg = segments[mid];

        if (x < seg.x1) {
            high = mid - 1;
        } else if (x > seg.x2) {
            low = mid + 1;
        } else {
            if (updateClearTails) {
                shouldClearTails = seg.clearsTails && lastx < seg.x1; // xpos has crossed this segment, clear reverb tail
                lastx = x;
            }
            if (seg.type == PointType::Hold) return seg.y1; // hold
            if (seg.type == PointType::Curve) return get_y_curve(seg, x);
            if (seg.type == PointType::SCurve) return get_y_scurve(seg, x);
            if (seg.type == PointType::Pulse) return get_y_pulse(seg, x);
            if (seg.type == PointType::Wave) return get_y_wave(seg, x);
            if (seg.type == PointType::Triangle) return get_y_triangle(seg, x);
            if (seg.type == PointType::Stairs) return get_y_stairs(seg, x);
            if (seg.type == PointType::SmoothSt) return get_y_smooth_stairs(seg, x);
            return -1;
        }
    }

    return -1;
}

void Pattern::createUndo()
{
    if (undoStack.size() > globals::MAX_UNDO) {
        undoStack.erase(undoStack.begin());
    }
    undoStack.push_back(points);
    redoStack.clear();
}
void Pattern::undo()
{
    if (undoStack.empty())
        return;

    {
        std::lock_guard<std::mutex> lock(pointsmtx);
        clearTransform();

        redoStack.push_back(points);
        points = undoStack.back();
        undoStack.pop_back();
    }

    incrementVersion();
    buildSegments();
}

void Pattern::redo()
{
    if (redoStack.empty())
        return;

    {
        std::lock_guard<std::mutex> lock(pointsmtx);
        clearTransform();

        undoStack.push_back(points);
        points = redoStack.back();
        redoStack.pop_back();

        incrementVersion();
    }

    buildSegments();
}

void Pattern::clearUndo()
{
    undoStack.clear();
    redoStack.clear();
}

bool Pattern::comparePoints(const std::vector<PPoint>& a, const std::vector<PPoint>& b)
{
    if (a.size() != b.size()) {
        return false;
    }
    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i].id != b[i].id ||
            a[i].x != b[i].x ||
            a[i].y != b[i].y ||
            a[i].tension != b[i].tension ||
            a[i].type != b[i].type) {
            return false;
        }
    }
    return true;
}