// :x-----------------------------------------
// optional: cut file in .h and .cpp part here
#include "vorosketch-037.h"
#include "node.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <queue>
#include <string.h>
#include <vector>
#include <filesystem>
/***********************************************************************

IMPLEMENTATION


*********************************************************/

/**********************************************************
 * General tools                                          *
 *********************************************************/
namespace vorosketch {

std::vector<double> site_change;

const DOUBLE SQRTTWO = sqrt((DOUBLE)2);

template <class T> std::vector<T> vectorOfTwo(T first, T second) {
  std::vector<T> v;
  v.push_back(first);
  v.push_back(second);
  return v;
}

template <class T> T **newMatrix(const long m, const long n) {
  T **A = new T *[m];
  A[0] = new T[m * n];
  for (int i = 1; i < m; ++i)
    A[i] = A[i - 1] + n;
  return A;
}

template <class T> void deleteMatrix(T **A) {
  delete[] A[0];
  delete[] A;
}

template <class T> T ***newMatrix(const long l, const long m, const long n) {
  T ***A = new T **[l];
  A[0] = new T *[l * m];
  for (int i = 1; i < l; ++i)
    A[i] = A[i - 1] + m;
  A[0][0] = new T[l * m * n];
  int lm = l * m;
  for (int i = 1; i < lm; ++i)
    A[0][i] = A[0][i - 1] + n;
  return A;
}

template <class T> void deleteMatrix(T ***A) {
  delete[] A[0][0];
  delete[] A[0];
  delete[] A;
}

DOUBLE &keepMin(DOUBLE &x, const DOUBLE y) {
  if (isnan(x) || y < x)
    x = y;
  return x;
}

DOUBLE &keepMax(DOUBLE &x, const DOUBLE y) {
  if (isnan(x) || y > x)
    x = y;
  return x;
}

DOUBLE safeMin(const DOUBLE x, const DOUBLE y) {
  return ((isnan(x) || y < x) ? y : x);
}

DOUBLE safeMax(const DOUBLE x, const DOUBLE y) {
  return ((isnan(x) || y > x) ? y : x);
}

bool nonDecreasing(const DOUBLE x, const DOUBLE y, const DOUBLE z) {
  return (x <= y && y <= z);
}

bool xyInDifferentBracketsOfz(const int x, const int y, const int z) {
  if ((x < 0) != (y < 0))
    return true;
  if (x < 0)
    return (x + 1) / z != (y + 1) / z;
  return x / z != y / z;
}

Range::Range(const DOUBLE _lowerBound, const DOUBLE _upperBound)
    : lowerBound(_lowerBound), upperBound(_upperBound) {};

Range &Range::operator+=(const Range &r) {
  lowerBound += r.lowerBound;
  upperBound += r.upperBound;

  // What if lw is now NaN? This can arise only in the following cases:
  // *  lw was NaN already. Then up must have been and must be NaN too, and
  //    this is the correct output.
  // *  r.lw was NaN already. Then r.up must have been and up must be NaN too,
  //    and this is the correct output.
  // *  lw was -INF, r.lw is INF, up > -INF. Then r.up must be INF too, up is
  //    now INF too, and this is the only valid value that the sum can have,
  //    so we should lw = up.
  // *  lw was -INF, r.lw is INF, up = -INF. Then r.up must be INF too, up is
  //    now NAN, and indeed, this is the correct output.
  // *  lw was INF, r.lw is -INF, r.up > -INF. Then up must be INF, and this is
  //    the only valid value that the sum can have, so we should set lw = up.
  // *  lw was INF, r.lw is -INF, r.up = -INF. Then up must have been INF too,
  //    and is now NAN, and indeed, this is the correct output.
  // So in all cases, this is safe:

  if (isnan(lowerBound))
    lowerBound = upperBound;

  // For the case that up is NaN, the situation is symmetric:

  if (isnan(upperBound))
    upperBound = lowerBound;

  return *this;
}

Range &Range::operator-=(const Range &r) { return operator+=(-r); }

Range Range::operator-() const { return Range(-upperBound, -lowerBound); }

Range &Range::erase() {
  lowerBound = NAN;
  upperBound = NAN;
  return *this;
}

Range &Range::extendTo(const DOUBLE x) {
  keepMin(lowerBound, x);
  keepMax(upperBound, x);
  return *this;
}

bool Range::contains(const DOUBLE x) const {
  return nonDecreasing(lowerBound, x, upperBound);
}

bool Range::isEmpty() const { return isnan(lowerBound); }

Range &Range::set(const DOUBLE x) {
  lowerBound = x;
  upperBound = x;
  return *this;
}

Range &Range::set(const DOUBLE l, const DOUBLE u) {
  lowerBound = l;
  upperBound = u;
  return *this;
}

bool Range::is(const DOUBLE l, const DOUBLE u) const {
  return (lowerBound == l && upperBound == u);
}

DOUBLE Range::size() const { return upperBound - lowerBound; }

DOUBLE Range::mean() const { return (lowerBound + upperBound) * 0.5; }

Range &keepMin(Range &x, const Range &y) {
  keepMin(x.lowerBound, y.lowerBound);
  keepMin(x.upperBound, y.upperBound);
  return x;
}

DOUBLE rnd(const DOUBLE atLeast, const DOUBLE lessThan) {
  return atLeast + ((DOUBLE)rand() / RAND_MAX) * (lessThan - atLeast);
}

DOUBLE sqr(const DOUBLE x) { return x * x; }

DOUBLE safeAcos(const DOUBLE x) {
  if (x >= 1)
    return 0;
  if (x <= -1)
    return PI;
  return acos(x);
}

DOUBLE safeAsin(const DOUBLE x) {
  if (x >= 1)
    return PI * 0.5;
  if (x <= -1)
    return PI * -0.5;
  return asin(x);
}

/*********************************************************
 * Input/Output                                           *
 *********************************************************/

const char *Complaint::errorLabel[4] = {"WARNING", "Error", "Error",
                                        "Internal error"};

Complaint::Complaint(ErrorLevel _errorLevel) : errorLevel(_errorLevel) {};

Complaint::Complaint(ErrorLevel _errorLevel, const std::string &expected,
                     const std::string &read)
    : errorLevel(_errorLevel) {
  complaint << "expected \"" << expected << "\", but read \"" << read << "\".";
}

Complaint::~Complaint() {
  std::cerr << std::endl
            << errorLabel[(int)errorLevel] << ": " << complaint.str()
            << std::endl;
  if (errorLevel == usageError)
    std::cerr << "Run vorosketch -? for usage." << std::endl;
  std::cerr << std::endl;
  if (errorLevel > warning)
    exit((int)errorLevel);
}

template <typename T> Complaint &Complaint::operator<<(const T &t) {
  complaint << t;
  return *this;
}

void makeLowerCase(std::string &s) {
  for (int i = 0; i < s.length(); ++i)
    s[i] = tolower(s[i]);
}

DOUBLE strtofstop(const std::string &str) {
  std::stringstream s(str);
  s >> std::ws;
  if (s.eof())
    Complaint(userError)
        << "expected a floating-point number but read nothing.";
  DOUBLE x;
  s >> x >> std::ws;
  if (!s.eof())
    Complaint(userError) << "expected a floating-point number but read \""
                         << str << "\".";
  return x;
}

int strtoistop(const std::string &str) {
  std::stringstream s(str);
  int n;
  s >> n >> std::ws;
  if (!s.eof())
    Complaint(userError) << "expected an integer but read \"" << str << "\".";
  return n;
}

void reportProgress(DOUBLE x, std::string description) {
  if (x >= 1)
    std::cerr << '\r' << description << ": done."
              << std::string(progressUnits - 3, ' ');
  else {
    std::cerr << '\r' << description << ": [";
    for (int i = 1; i < progressUnits; ++i) {
      if (x * progressUnits >= i)
        std::cerr << (i % 5 ? '-' : '+');
      else
        std::cerr << (i % 5 ? ' ' : ':');
    }
    std::cerr << "]";
  }
  std::cerr.flush();
}

RGB::RGB(const int _red, const int _green, const int _blue)
    : mRed(_red), mGreen(_green), mBlue(_blue) {};

RGB::RGB(const double _grey)
    : mRed(_grey * 255), mGreen(_grey * 255), mBlue(_grey * 255) {};

const int RGB::red() const { return mRed; }

const int RGB::green() const { return mGreen; }

const int RGB::blue() const { return mBlue; }

RGB &RGB::operator*=(const RGB &colour) {
  mRed *= colour.mRed;
  mRed /= 255;
  mGreen *= colour.mGreen;
  mGreen /= 255;
  mBlue *= colour.mBlue;
  mBlue /= 255;
  return *this;
}

RGB RGB::operator*(const DOUBLE value) const {
  return RGB(mRed * value, mGreen * value, mBlue * value);
}

RGB &RGB::operator*=(const DOUBLE value) {
  mRed *= value;
  mGreen *= value;
  mBlue *= value;
  return *this;
}

void RGB::limit() {
  mRed = std::max(0, std::min(255, mRed));
  mGreen = std::max(0, std::min(255, mGreen));
  mBlue = std::max(0, std::min(255, mBlue));
}

RGB RGB::black(0, 0, 0);
RGB RGB::white(255, 255, 255);

const char *colourSchemes = "\
  palette trubetskoy-default site 255 255 255 sites 19 \
  230  25  75   60 180  75  255 225  25    0 130 200 \
  245 130  48  145  30 180   70 240 240  240  50 230 \
  210 245  60  250 190 212    0 128 128  220 190 225 \
  170 110  40  255 250 200  128   0   0  170 255 195 \
  128 128   0  255 215 180  128 128 128 \
  palette trubetskoy-original site 255 255 255 sites 20 \
  230  25  75   60 180  75  255 225  25    0 130 200 \
  245 130  48  145  30 180   70 240 240  240  50 230 \
  210 245  60  250 190 212    0 128 128  220 190 225 \
  170 110  40  255 250 200  128   0   0  170 255 195 \
  128 128   0  255 215 180    0   0 128  128 128 128 \
  palette trubetskoy-modified site 255 255 255 sites 20 \
  255  25 125   60 180  75  255 225  25    0 130 200 \
  245 130  48  145  30 180   70 240 240  240  50 230 \
  210 245  60  250 190 212    0 128 128  220 190 225 \
  170 110  40  255 250 200  192   0   0  170 255 195 \
  128 128   0  255 215 180   60  60 255  128 128 128 \
  palette grey site 255 255 255 sites 6 \
  128 128 128  179 179 179   77  77  77  153 153 153 \
  102 102 102  217 217 217 \
  palette stone site 255 255 255 sites 20 \
  136 112 112  187 163 163   93  69  69  161 137 137 \
  136 136 112  187 187 163   93  93  69  161 161 137 \
  112 112 136  163 163 187   69  69  93  137 137 161 \
  112 136 136  163 187 187   69  93  93  137 161 161 \
  136 112 136  187 163 187   93  69  93  161 137 161 \
  palette brightstone site 255 255 255 sites 19 \
  136 112 112  187 163 163  116  86  86  161 137 137 \
  136 136 112  187 187 163  116 116  86  161 161 137 \
  112 112 136  163 163 187               137 137 161 \
  112 136 136  163 187 187   86 116 116  137 161 161 \
  136 112 136  187 163 187  116  86 116  161 137 161 \
  palette flat site 255 255 255 sites 12 \
  255  28  83   79 238  99  172 201  49    0 147 226 \
  234 124  46  168  35 208   54 186 186  208  40 184 \
  178 135 151  168 164 132  126 189 145   70  70 255 \
  palette bright site 255 255 255 sites 12 \
  255 255   0    0 192 255  255   0 128    0 255  96 \
  160   0 255  255 144   0  208   0 255    0 128 255 \
  255 192   0  255   0 255  192 255   0    0 255 255 \
";

std::istream &operator>>(std::istream &s, RGB &rgb) {
  return s >> rgb.mRed >> rgb.mGreen >> rgb.mBlue;
}

DOUBLE contrast(const RGB &a, const RGB &b) {
  return abs(sqr(a.red()) - sqr(b.red())) +
         abs(sqr(a.green()) - sqr(b.green())) +
         abs(sqr(a.blue()) - sqr(b.blue()));
}

RGB *chooseColours(Options &options, const int nrGroups, bool **adjacent) {
  // initial colouring:
  int colourId[nrGroups];
  for (int i = 0; i < nrGroups; ++i)
    colourId[i] = i % options.colours.sites.size();

  if (options.colourAssignment != Options::inputOrder) {
    // maximum allowed iterations for cubic-time improvement round:
    int maxIterations = 1 + (1L << 38) / ((long)nrGroups * nrGroups * nrGroups);
    {
      DOUBLE **diff = newMatrix<DOUBLE>(options.colours.sites.size(),
                                        options.colours.sites.size());
      for (int i = 0; i < options.colours.sites.size(); ++i)
        for (int j = 0; j <= i; ++j) {
          diff[i][j] =
              contrast(options.colours.sites[i], options.colours.sites[j]);
          diff[j][i] = diff[i][j];
        }

      // try to improve:
      bool improved = true;
      for (int iteration = 0; improved && (iteration < maxIterations);
           ++iteration) {
        improved = false;
        std::stringstream progressLabel;
        progressLabel << "> Swapping colours (round " << iteration + 1 << ')';

        // try swapping two colours:
        for (int i = nrGroups - 1; i > 0; --i) {
          if (options.verbose)
            reportProgress(((DOUBLE)nrGroups - i) / nrGroups,
                           progressLabel.str());
          for (int j = i - 1; j >= 0; --j) {
            if (options.colourAssignment == Options::highContrast) {
              // try to improve contrast:
              DOUBLE before = INF;
              DOUBLE after = INF;
              for (int k = 0; k < nrGroups; ++k) {
                if (k == i || k == j)
                  continue;
                if (adjacent[i][k]) {
                  keepMin(before, diff[colourId[i]][colourId[k]]);
                  keepMin(after, diff[colourId[j]][colourId[k]]);
                }
                if (adjacent[j][k]) {
                  keepMin(before, diff[colourId[j]][colourId[k]]);
                  keepMin(after, diff[colourId[i]][colourId[k]]);
                }
              }
              if (after <= before)
                continue;
            } else if (options.colourAssignment == Options::lowContrast) {
              // try to reduce contrast:
              DOUBLE before = 0;
              DOUBLE after = 0;
              for (int k = 0; k < nrGroups; ++k) {
                if (k == i || k == j)
                  continue;
                if (adjacent[i][k]) {
                  before += diff[colourId[i]][colourId[k]];
                  after += diff[colourId[j]][colourId[k]];
                }
                if (adjacent[j][k]) {
                  before += diff[colourId[j]][colourId[k]];
                  after += diff[colourId[i]][colourId[k]];
                }
              }
              if (after >= before)
                continue;
            } else
              Complaint(internalError) << "unknown colour optimisation mode";
            std::swap(colourId[i], colourId[j]);
            improved = true;
            break;
          }
        }
        if (options.verbose)
          reportProgress(1, progressLabel.str());
      }
      deleteMatrix<DOUBLE>(diff);
    }
  }

  RGB *colour = new RGB[nrGroups + 1];
  for (int i = 0; i < nrGroups; ++i)
    colour[i] = options.colours.sites[colourId[i]];
  colour[nrGroups] = options.colours.unclaimed;

  if (options.fadeByDegree) {
    for (int i = 0; i < nrGroups; ++i) {
      long degree = 0;
      for (int j = 0; j < nrGroups; ++j)
        if (adjacent[i][j])
          ++degree;
      degree = sqr(sqr(degree));
      DOUBLE w = 1.5 / (1.0 + 2500.0 / degree);
      colour[i] = RGB(w * colour[i].red() + (1.0 - w) * 255,
                      w * colour[i].green() + (1.0 - w) * 255,
                      w * colour[i].blue() + (1.0 - w) * 255);
    }
  }

  return colour;
}

BitmapFile::BitmapFile(std::ostream &_outputStream, const int _width,
                       const int height)
    : outputStream(_outputStream), width(_width),
      samplesYetToCome(_width * height), padding((4 - ((3 * width) % 4)) % 4) {
  // write header:
  const int headerSize = 26;
  const int lineSize = width * 3 + padding;
  const int fileSize = headerSize + height * lineSize;
  writeInteger(0x42, 1);
  writeInteger(0x4D, 1);
  writeInteger(fileSize, 4);
  writeInteger(0, 4);
  writeInteger(headerSize, 4);
  writeInteger(12, 4);
  writeInteger(width, 2);
  writeInteger(height, 2);
  writeInteger(1, 2);
  writeInteger(24, 2);
}

BitmapFile &BitmapFile::writePixel(RGB colour) {
  if (--samplesYetToCome < 0)
    Complaint(internalError) << "attempt to write to finished bitmap file";
  colour.limit();
  writeInteger(colour.blue(), 1);
  writeInteger(colour.green(), 1);
  writeInteger(colour.red(), 1);
  if (samplesYetToCome % width == 0)
    writeInteger(0, padding);
  return *this;
}

BitmapFile::~BitmapFile() {
  if (samplesYetToCome > 0)
    Complaint(internalError) << "bitmap file incomplete";
}

/*********************************************************
 * Geometry                                               *
 *********************************************************/

Point::Point(const DOUBLE _x, const DOUBLE _y) : x(_x), y(_y) {};

bool Point::operator==(const Point &p) const { return (x == p.x && y == p.y); }

bool Point::operator!=(const Point &p) const { return (x != p.x || y != p.y); }

bool Point::inOrigin() const { return (x == 0 && y == 0); }

Point Point::normalised(DOUBLE targetLength) const {
  if (targetLength < 0)
    Complaint(internalError) << "normalised called with negative length";
  return operator*(targetLength / length());
}

Point Point::operator+(const Point &p) const { return Point(x + p.x, y + p.y); }

Point Point::operator-(const Point &p) const { return Point(x - p.x, y - p.y); }

Point Point::operator*(const DOUBLE scalar) const {
  return Point(x * scalar, y * scalar);
}

Point &Point::operator*=(const DOUBLE scalar) {
  x *= scalar;
  y *= scalar;
  return *this;
}

Point Point::operator/(const DOUBLE scalar) const {
  return Point(x / scalar, y / scalar);
}

Point &Point::operator/=(const DOUBLE scalar) {
  x /= scalar;
  y /= scalar;
  return *this;
}

DOUBLE Point::operator*(const Point &p) const { return x * p.x + y * p.y; }

DOUBLE Point::sqr() const { return x * x + y * y; }

DOUBLE Point::length() const { return sqrt(sqr()); }

DOUBLE Point::fi() const {
  if (x == 0 && y == 0)
    return NAN;
  return atan2(y, x);
}

DOUBLE Point::ySlope() const { return y / x; }

DOUBLE Point::xSlope() const { return x / y; }

Point Point::rotLeft() const { return Point(-y, x); }

Point min(const std::vector<Point> &points) {
  Point m(INF, INF);
  for (std::vector<Point>::const_iterator p = points.begin(); p != points.end();
       ++p) {
    keepMin(m.x, p->x);
    keepMin(m.y, p->y);
  }
  return m;
}

Point max(const std::vector<Point> &points) {
  Point m(-INF, -INF);
  for (std::vector<Point>::const_iterator p = points.begin(); p != points.end();
       ++p) {
    keepMax(m.x, p->x);
    keepMax(m.y, p->y);
  }
  return m;
}

std::istream &operator>>(std::istream &is, Point &p) {
  return is >> p.x >> p.y;
}

DOUBLE cosAngle(const Point &p1, const Point &p2, const Point &p3) {
  Point v1 = p1 - p2;
  Point v3 = p3 - p2;
  return (v1 * v3) / v1.length() / v3.length();
}

void getTurnRange(const Point &centre, DOUBLE halfwidth, const Point &direction,
                  Range &range) {
  if (halfwidth == 0)
    Complaint(internalError) << "calling getTurnRange with halfwidth=0";

  if (direction.x == 0 && direction.y == 0) {
    range.set(0, PI);
    return;
  }

  // if the ray cuts the square, the lower bound is 0
  // if the opposite ray cuts the square, the upper bound is PI
  // otherwise the lower and upper bounds are attained at corners

  // to determine if the ray cuts the square: use left turn distance
  // the ray cuts the square if lower and upper bound are >= PI apart

  Range fwd(TWOPI, 0);
  Range rev(TWOPI, 0);

  Point corner[4] = {centre + Point(-halfwidth, -halfwidth),
                     centre + Point(-halfwidth, halfwidth),
                     centre + Point(halfwidth, -halfwidth),
                     centre + Point(halfwidth, halfwidth)};
  for (int i = 0; i < 4; ++i) {
    if (corner[i].inOrigin())
      continue;

    DOUBLE angle = 0;
    // rotate such that direction is rotated onto positive x-axis
    // (the transformation also scales, but that does not matter)
    Point n(direction * corner[i], direction.rotLeft() * corner[i]);
    angle = atan2(n.y, n.x);

    // angle lies between -pi and pi
    keepMin(rev.lowerBound, angle + PI);
    keepMax(rev.upperBound, angle + PI);
    if (angle < 0)
      angle += TWOPI;
    keepMin(fwd.lowerBound, angle);
    keepMax(fwd.upperBound, angle);
  }

  range.lowerBound = ((fwd.upperBound - fwd.lowerBound >= PI)
                          ? 0
                          : std::min(fwd.lowerBound, TWOPI - fwd.upperBound));
  range.upperBound = ((rev.upperBound - rev.lowerBound >= PI)
                          ? PI
                          : std::min(fwd.upperBound, TWOPI - fwd.lowerBound));
}

DOUBLE safeCosXY1WithSun(const DOUBLE x, const DOUBLE y)
// computes cosine of vector (x,y,1) and unit-length sun vector
// -1 if (x,y,1).v = -inf;
//  0 if (x,y,1).v is not a number;
//  1 if (x,y,1).v =  inf
{
  static const SphericalPoint sun(-1.0 / 3.0, 2.0 / 3.0, 2.0 / 3.0);
  DOUBLE dotProduct = (x * sun.x + y * sun.y + sun.z);
  return dotProduct == -INF  ? -1
         : isnan(dotProduct) ? 0
         : dotProduct == INF ? 1
                             : dotProduct / sqrt(sqr(x) + sqr(y) + 1);
}

SphericalPoint::SphericalPoint() : x(0), y(0), z(0) {};

SphericalPoint::SphericalPoint(const DOUBLE _x, const DOUBLE _y,
                               const DOUBLE _z)
    : x(_x), y(_y), z(_z) {};

SphericalPoint::SphericalPoint(const DOUBLE latitude, const DOUBLE longitude) {
  DOUBLE r = cos(latitude);
  x = r * cos(longitude);
  y = r * sin(longitude);
  z = sin(latitude);
}

DOUBLE SphericalPoint::operator*(const SphericalPoint &p) const {
  return x * p.x + y * p.y + z * p.z;
}

void SphericalPoint::stretchHemisphereToSphere() {
  // first rotate:
  DOUBLE w = x;
  x = z;
  z = y;
  y = w;

  // (cos lng cos lat,           sin lng cos lat,           sin lat) ->
  // ((2 cos^2 lng - 1) cos lat, 2 cos lng sin lng cos lat, sin lat)
  DOUBLE coslng = x / sqrt((DOUBLE)1.0 - sqr(z));
  y *= (coslng * 2);
  x *= ((sqr(coslng) * 2 - 1) / coslng);
}

void getCentreOfSphericalRectangle(const Range &height, const Range &longitude,
                                   SphericalPoint &centre, DOUBLE &radius)
// ! not tight if longitude range exceeds pi
{
  radius = -1; // default answer
  if (height.isEmpty())
    return;

  // Consider the Cartesian product of
  // a longitude interval and a latitude interval, w.l.o.g. let these
  // be [g-dg,g+dg] and [t1,t2]. As a centre point p we take the point
  // with longitude g, latitude t in [t1,t2], such that the distance
  // to the corners of the square (on the sphere) is equal.
  // Since this is invariant under rotation, assume g = 0, then p is
  // (cos t, 0, sin t) and the corners on one side are (cos ti cos dg,
  // cos ti sin dg, sin ti) for i = 1, 2; they are at equal distance if:
  // cos t cos t1 cos dg + sin t sin t1 = cos t cos t2 cos dg + sin t sin t2
  // <=> tan t = cos dg (cos t2 - cos t1) / (sin t1 - sin t2).
  // By analysing the derivatives w.r.t t1, t2, dg, one can verify, for
  // dg <= pi/2, that the corners are the points in the rectangle furthest
  // from p.
  // If dg > pi/2, these bounds are not correct, but zooming out so far
  // that we get a substantial number of grid cells with dg > pi/2 would
  // not make any sense---so it's ok to use more trivial bounds there.

  DOUBLE g = longitude.mean();
  DOUBLE dg = longitude.size() * 0.5;
  DOUBLE cosdg = cos(dg);
  if (cosdg < 0) // extreme longitude range: larger than PI
  {
    if (height.lowerBound > 0) // centre on north pole
    {
      centre = SphericalPoint(0, 0, 1);
      radius = HALFPI - safeAsin(height.lowerBound);
    } else // centre on south pole
    {
      centre = SphericalPoint(0, 0, -1);
      radius = safeAsin(height.upperBound) + HALFPI;
    }
  } else {
    DOUBLE cost1 = sqrt(1 - sqr(height.lowerBound));
    DOUBLE cost2 = sqrt(1 - sqr(height.upperBound));
    DOUBLE t = atan(cosdg * (cost1 - cost2) / height.size());
    DOUBLE sint = sin(t);
    DOUBLE cost = cos(t);
    centre = SphericalPoint(cost * cos(g), cost * sin(g), sint);
    radius = safeAcos(cost * cost1 * cosdg + sint * height.lowerBound);
  }
}

GansPoint::GansPoint(const DOUBLE _x, const DOUBLE _y) : x(_x), y(_y) {};

GansPoint &GansPoint::operator=(const Point &p) {
  x = p.x;
  y = p.y;
  return *this;
}

DOUBLE coshdistance(const GansPoint &p, const GansPoint &q) {
  return sqrt((sqr(p.x) + sqr(p.y) + 1) * (sqr(q.x) + sqr(q.y) + 1)) -
         p.x * q.x - p.y * q.y;
}

/*********************************************************
 * Options                                                *
 *********************************************************/

Colours::Colours()
    : sites(1, RGB::white), unclaimed(RGB::black), softEdgeShade(RGB(0.977)),
      bisector(RGB::black), majorContour(RGB::black),
      minorContourShade(RGB(0.880)), obstacle(RGB::black), highway(RGB(0.755)),
      highwayOutline(RGB::black), site(RGB::black), siteOutline(RGB::black),
      origin(RGB::black), originOutline(RGB::white), node(RGB::black),
      grid(RGB::white) {
  gradient.push_back(RGB(0, 0, 0));
  gradient.push_back(RGB(0, 0, 0));
  gradient.push_back(RGB(0, 0, 64));
  gradient.push_back(RGB(0, 0, 144));
  gradient.push_back(RGB(96, 0, 255));
  gradient.push_back(RGB(224, 0, 224));
  gradient.push_back(RGB(255, 0, 128));
  gradient.push_back(RGB(255, 0, 48));
  gradient.push_back(RGB(255, 96, 0));
  gradient.push_back(RGB(255, 255, 0));
  gradient.push_back(RGB(255, 255, 255));
};

void Colours::read(std::istream &repo, std::string name) {
  makeLowerCase(name);
  std::string header;
  do {
    do {
      repo >> header;
      makeLowerCase(header);
    } while (repo && header != "palette");
    if (!repo)
      Complaint(usageError) << "colour palette \"" << name << "\" not found";
    repo >> header;
    makeLowerCase(header);
  } while (header != name);
  while (repo) {
    repo >> header;
    makeLowerCase(header);
    if (header == "palette") // next palette
      break;
    else if (header == "sites") {
      int nrColours;
      repo >> nrColours;
      if (nrColours < 1)
        Complaint(usageError) << "invalid number of site colours (" << nrColours
                              << ") in palette";
      sites.resize(nrColours);
      for (int i = 0; i < nrColours; ++i)
        repo >> sites[i];
    } else if (header == "gradient") {
      int nrColours;
      repo >> nrColours;
      if (nrColours < 2)
        Complaint(usageError) << "invalid number of gradient colours ("
                              << nrColours << ") in palette";
      gradient.resize(nrColours);
      for (int i = 0; i < nrColours; ++i)
        repo >> gradient[i];
    } else if (header == "unclaimed")
      repo >> unclaimed;
    else if (header == "softedge")
      repo >> softEdgeShade;
    else if (header == "bisector")
      repo >> bisector;
    else if (header == "majorcontour")
      repo >> majorContour;
    else if (header == "minorcontour")
      repo >> minorContourShade;
    else if (header == "obstacle")
      repo >> obstacle;
    else if (header == "highway")
      repo >> highway;
    else if (header == "highwayoutline")
      repo >> highwayOutline;
    else if (header == "site")
      repo >> site;
    else if (header == "siteoutline")
      repo >> siteOutline;
    else if (header == "origin")
      repo >> origin;
    else if (header == "originoutline")
      repo >> originOutline;
    else if (header == "node")
      repo >> node;
    else if (header == "grid")
      repo >> grid;
  }
}

// first declare some helpers:

const OptionSyntax *findOption(const OptionSyntax options[], std::string argv);

struct Argument
// class to store options with arguments;
// different options must have unique ranks and ids
{
  int position; // position in the input
  const OptionSyntax *key;
  std::string value;
  Argument(int _position, const OptionSyntax &_key, const std::string &_value);

  // functions to extract a number from the std::string value, and
  // verify that the number is within given bounds:
  int intValue(const int min, const int max) const;
  DOUBLE floatValue(const DOUBLE min, const DOUBLE max) const;

  // comparison operator to sort by rank, and for options of equal rank,
  // by position in the input:
  bool operator<(const Argument &a) const;
};

// and here is the implementation:

OptionSyntax::OptionSyntax(int _rank, std::string _id, bool _requiresValue,
                           bool _multipleAllowed)
    : rank(_rank), id(_id), requiresArgument(_requiresValue),
      multipleAllowed(_multipleAllowed) {};

// Options::Options(const int argc, const char* argv[]):
Options::Options()
    : distance(new EuclideanDistance()), useEuclideanHighways(false),
      useManhattanHighways(false), useCompositeDistance(false),
      weighting(Options::subtractive), equalityThreshold(0),
      segmentShortening(0), order(Options::first), drawSites(true),
      drawRegionBoundaries(false), arrowLength(0),
      colourAssignment(Options::inputOrder), fadeByDegree(false),
      unicolourSites(true), drawStandardContours(false), contourInterval(0),
      shadeInnerAreas(true), contourDistance(0), emphasizeFifth(false),
      halfBrightDistance(-1), gradient(noGradient), sunlight(false),
      cutOffDistance(INF), distanceInfoNeeded(true), width(2000),
      markOrigin(false), windowWidth(2), windowOffset(-1),
      pixelHalfWidth(0.0005), pixelsPerUnit(1000), randomSites(0),
      noSegmentSites(false), predefinedSites(0), verbose(false),
      generateImage(false) // -j generates image
{};

void Options::parse(const int argc, const char *argv[]) {
  // collect command line arguments, check syntax
  std::vector<Argument> arguments;
  for (int i = 1; i < argc; ++i) {
    std::string s(argv[i]);
    const OptionSyntax *o = findOption(syntax, s);
    if (!o) {
      if (s[0] != '-')
        Complaint(usageError)
            << "unrecognised command line parameter \"" << argv[i] << "\".";

      // maybe it is multiple options after a single dash, first parse all but
      // the last one:
      for (; s.length() > 2; s.erase(1, 1)) {
        o = findOption(syntax, s.substr(0, 2));
        if (!o)
          Complaint(usageError)
              << "unrecognised option \"" << s.substr(0, 2) << "\".";
        if (o->requiresArgument)
          Complaint(usageError)
              << "missing value for \"" << o->id << "\" option.";
        arguments.push_back(Argument(arguments.size(), *o, ""));
      }

      // now identify the last option
      o = findOption(syntax, s);
      if (!o)
        Complaint(usageError) << "unrecognised option \"" << s << "\".";
    }
    if (o->requiresArgument) {
      if (++i == argc)
        Complaint(usageError)
            << "missing value for \"" << o->id << "\" option.";
      arguments.push_back(Argument(arguments.size(), *o, argv[i]));
    } else
      arguments.push_back(Argument(arguments.size(), *o, ""));
  }

  // sort command line arguments, check for missing or double arguments
  sort(arguments.begin(), arguments.end());
  for (int i = 1; i < arguments.size(); ++i)
    if (arguments[i].key->rank == arguments[i - 1].key->rank &&
        arguments[i].value != arguments[i - 1].value &&
        !arguments[i].key->multipleAllowed)
      Complaint(usageError)
          << "multiple values for \"" << arguments[i].key->id + "\" option (\""
          << arguments[i - 1].value << "\" and \"" << arguments[i].value
          << "\").";

  // process command line arguments
  for (std::vector<Argument>::iterator a = arguments.begin();
       a != arguments.end(); ++a) {
    switch (a->key->id[1]) {
    case 'm':
      delete distance;
      {
        std::string name(a->value);
        makeLowerCase(name);
        noSegmentSites = (name.find("l1") != std::string::npos ||
                          name.find("l2") != std::string::npos);
        distance = getDistanceObject(name);
        if (!distance->supportedSites())
          Complaint(userError) << "Distance function is not compatible with or "
                                  "has not been implemented "
                               << "for any type of site";
      }
      break;
    case 'd':
      weighting = divisive;
      contourDistance = 1;
      break;
    case '2':
      order = second;
      shadeInnerAreas = false;
      break;
    case 'n':
      order = nextclosest;
      shadeInnerAreas = false;
      break;
    case 'f':
      order = farthest;
      shadeInnerAreas = false;
      break;
    case 'e':
      if (order == second)
        Complaint(warning)
            << "-e option not supported for second-order Voronoi Diagrams.";
      else if (order == nextclosest)
        Complaint(warning)
            << "-e option not supported for next-closest Voronoi Diagrams.";
      else
        equalityThreshold = a->floatValue(0, 1) * windowWidth / width;
      break;
    case 's':
      if (!distance->supports(SEGMENTTYPE))
        Complaint(warning) << "-s option irrelevant or not supported for "
                           << distance->description() << " distance.";
      else
        segmentShortening = a->floatValue(0, 1) * windowWidth / width;
      break;
    case 'a':
      drawSites = false;
      break;
    case 'j':
      generateImage = true;
      break;
    case 'b':
      drawRegionBoundaries = true;
      break;
    case 'c':
      a->value = "Trubetskoy-Default";
      // no break, continue as with -p option:
    case 'p': {
      std::string name(a->value);
      int colon = name.find(':');
      if (colon == std::string::npos) {
        std::stringstream s(colourSchemes);
        colours.read(s, name);
      } else {
        std::ifstream s(name.substr(0, colon));
        if (!s)
          Complaint(userError) << "cannot open colour palette file \""
                               << name.substr(0, colon) << "\"";
        name.erase(0, colon + 1);
        colours.read(s, name);
      }
      unicolourSites = false;
    } break;
    case 'h':
      colourAssignment = highContrast;
      break;
    case 'l':
      colourAssignment = lowContrast;
      break;
    case 'x':
      fadeByDegree = true;
      break;
    case '*':
      unicolourSites = true;
      break;
    case 'o':
      shadeInnerAreas = false;
      break;
    case 'u':
      drawStandardContours = true;
      break;
    case 'i':
      contourInterval = a->floatValue(0, 100);
      break;
    case '5':
      emphasizeFifth = true;
      break;
    case 'g':
      halfBrightDistance = a->floatValue(0, 100);
      shadeInnerAreas = false;
      gradient = brightnessGradient;
      break;
    case 'k':
      if (gradient == brightnessGradient)
        Complaint(usageError) << "-g and -k options are mutually exclusive.";
      if (colourAssignment == highContrast)
        Complaint(usageError) << "-h and -k options are mutually exclusive.";
      if (colourAssignment == lowContrast)
        Complaint(usageError) << "-k and -l options are mutually exclusive.";
      if (fadeByDegree)
        Complaint(usageError) << "-k and -x options are mutually exclusive.";
      halfBrightDistance = a->floatValue(0, 100);
      shadeInnerAreas = false;
      gradient = colourGradient;
      unicolourSites = true;
      break;
    case 'z':
      if (order == second)
        Complaint(warning)
            << "-z option not supported for second-order Voronoi Diagrams.";
      if (gradient == brightnessGradient)
        Complaint(warning) << "-g and -z options are mutually exclusive.";
      sunlight = true;
      break;
    case 't':
      cutOffDistance = a->floatValue(0, 1000);
      break;
    case 'r':
      width = a->intValue(2, MAXRESOLUTION);
      break;
    case 'v':
      verbose = true;
      break;
    case '?':
      usage();
      break;
    case '+':
      markOrigin = true;
      break;
    case 'w':
      windowWidth = a->floatValue(0.001, 1000) * 2;
      break;
    case '@':
      if (predefinedSites != 0)
        Complaint(usageError) << "-@ and -# options are mutually exclusive.";
      randomSites = a->intValue(1, MAXNRGROUPS);
      break;
    case '#':
      if (randomSites != 0)
        Complaint(usageError) << "-@ and -# options are mutually exclusive.";
      predefinedSites = a->intValue(2, 79);
      break;
    default:
      Complaint(internalError) << "unrecognized option";
    }
  }
  if ((colours.sites.size() == 1 || gradient == colourGradient) && !sunlight)
    drawRegionBoundaries = true;
  windowOffset = -windowWidth / 2;
  pixelHalfWidth = windowWidth / width / 2;
  pixelsPerUnit = width / windowWidth;
  if (distance->supportedSites() == ROOTEDVECTORTYPE ||
      distance->supportedSites() == DIRECTEDPOINTTYPE)
    arrowLength = 0.03 * windowWidth;
  distanceInfoNeeded =
      (drawStandardContours || contourInterval > 0 || shadeInnerAreas ||
       halfBrightDistance > 0 || sunlight || cutOffDistance < INF);
  if (predefinedSites > 0 && !distance->supports(SPHERICALPOINTTYPE) &&
      !distance->supports(HYPERBOLICPOINTTYPE))
    Complaint(usageError)
        << "predefined sites are only available for distance measures that "
           "support spherical or hyperbolic points.";
  if (verbose)
    report(std::cerr);
}

void Options::report(std::ostream &s) const {
  std::cerr << "Order: ";
  if (order == first)
    std::cerr << "1 (regions determined by closest site)" << std::endl;
  else if (order == second)
    std::cerr << "2 (regions determined by closest two sites)" << std::endl;
  else if (order == nextclosest)
    std::cerr << "regions determined by second-closest site" << std::endl;
  else if (order == farthest)
    std::cerr << "n-1 (regions determined by farthest site)" << std::endl;
  else
    Complaint(internalError) << "order set incorrectly";

  std::cerr << "Distance measure: " << distance->description() << std::endl;

  std::cerr << "Weighting: ";
  if (weighting == subtractive)
    std::cerr << "subtract weights" << std::endl;
  else if (weighting == divisive)
    std::cerr << "divide by weights" << std::endl;
  else
    Complaint(internalError) << "weighting method set incorrectly";

  std::cerr << "Distances considered equal if difference at most: "
            << equalityThreshold << std::endl;

  if (distance->supports(SEGMENTTYPE))
    std::cerr << "Segment/polyline sites shortened by: " << segmentShortening
              << std::endl;

  std::cerr << "Image size: " << width << 'x' << width << " pixels"
            << std::endl;

  std::cerr << "Window: [" << windowOffset << ',' << windowWidth + windowOffset
            << "]x[" << windowOffset << ',' << windowWidth + windowOffset << "]"
            << std::endl;
  std::cerr << "Origin: ";
  if (!markOrigin)
    std::cerr << "not ";
  std::cerr << "marked" << std::endl;

  std::cerr << "Regions: ";
  if (gradient == colourGradient)
    std::cerr << "coloured by distance from site" << std::endl;
  else if (colours.sites.size() == 1)
    std::cerr << "white" << std::endl;
  else if (colours.sites.size() > 1)
    std::cerr << "in up to " << colours.sites.size() << " colours" << std::endl;
  else
    Complaint(internalError) << "colour scheme set incorrectly";

  std::cerr << "Sites: ";
  if (drawSites == false)
    std::cerr << "not drawn" << std::endl;
  else if (colours.sites.size() == 1 || unicolourSites)
    std::cerr << "all the same" << std::endl;
  else
    std::cerr << "in matching colours" << std::endl;

  if (gradient != colourGradient && colours.sites.size() > 1) {
    std::cerr << "Colour assignment: ";
    if (colourAssignment == inputOrder)
      std::cerr << "round-robin in input order";
    else if (colourAssignment == highContrast)
      std::cerr << "maximise contrast between adjacent regions";
    else if (colourAssignment == lowContrast)
      std::cerr << "minimise contrast between adjacent regions";
    else
      Complaint(internalError) << "colour assignment set incorrectly";
    if (fadeByDegree)
      std::cerr << "; fade regions with few neighbours";
    std::cerr << std::endl;
  }

  std::cerr << "Unit distance contours: ";
  if (drawStandardContours) {
    std::cerr << "distance " << contourDistance << " marked by contour line";
    if (shadeInnerAreas)
      std::cerr << ", area within shaded";
    std::cerr << std::endl;
  } else if (shadeInnerAreas)
    std::cerr << "area within distance " << contourDistance << " shaded"
              << std::endl;
  else
    std::cerr << "not shown" << std::endl;

  std::cerr << "Outer contour: ";
  if (cutOffDistance < INF)
    std::cerr << "area outside distance " << cutOffDistance << " blackened"
              << std::endl;
  else
    std::cerr << "none" << std::endl;

  std::cerr << "Further contour lines: ";
  if (contourInterval > 0) {
    std::cerr << "at " << contourInterval << " intervals";
    if (emphasizeFifth)
      std::cerr << ", every 5th emphasized";
    std::cerr << std::endl;
  } else
    std::cerr << "none" << std::endl;

  std::cerr << "Shading: ";
  if (sunlight)
    std::cerr << "by distance gradient";
  if (halfBrightDistance != -1) {
    if (sunlight)
      std::cerr << ", ";
    if (gradient == brightnessGradient)
      std::cerr << "50% brightness";
    else if (gradient == colourGradient)
      std::cerr << "median colour";
    else
      Complaint(internalError) << "shading method set incorrectly";
    std::cerr << " at distance " << halfBrightDistance << std::endl;
  } else if (shadeInnerAreas) {
    if (sunlight)
      std::cerr << ", ";
    std::cerr << "dark shade within distance " << contourDistance << std::endl;
  } else {
    if (!sunlight)
      std::cerr << "none";
    std::cerr << std::endl;
  }
}

void Options::usage() {
  std::cerr << vorosketchInfo << std::endl;
#ifdef INCLUDE_USER_DISTANCES
  std::cerr << "extended with user-defined distance measures" << std::endl;
#endif
  std::cerr << std::endl
            << "USAGE    vorosketch [<options>]" << std::endl
            << "INPUT    sites in file format described below" << std::endl
            << "OUTPUT   bmp image file" << std::endl
            << std::endl
            << vorosketchOptions << std::endl
            << vorosketchFileFormat << std::endl;
  exit(-1);
}

bool Options::lineWork() const {
  return drawRegionBoundaries || drawStandardContours || contourInterval > 0;
}

const OptionSyntax Options::syntax[32] = {
    OptionSyntax(0, "-?"),        OptionSyntax(1, "-i", true),
    OptionSyntax(2, "-5"),        OptionSyntax(3, "-m", true),
    OptionSyntax(4, "-d"),        OptionSyntax(5, "-2"),
    OptionSyntax(6, "-n"),        OptionSyntax(7, "-f"),
    OptionSyntax(8, "-r", true),  OptionSyntax(9, "-w", true),
    OptionSyntax(10, "-e", true), OptionSyntax(11, "-s", true),
    OptionSyntax(12, "-c"),       OptionSyntax(13, "-p", true),
    OptionSyntax(14, "-h"),       OptionSyntax(15, "-l"),
    OptionSyntax(16, "-x"),       OptionSyntax(17, "-*"),
    OptionSyntax(18, "-a"),       OptionSyntax(19, "-b"),
    OptionSyntax(20, "-o"),       OptionSyntax(21, "-u"),
    OptionSyntax(22, "-g", true), OptionSyntax(23, "-k", true),
    OptionSyntax(24, "-z"),       OptionSyntax(25, "-t", true),
    OptionSyntax(26, "-+"),       OptionSyntax(27, "-@", true),
    OptionSyntax(28, "-#", true), OptionSyntax(29, "-j"),
    OptionSyntax(98, "-v"),       OptionSyntax(99, "") // sentinel end
};

const OptionSyntax *findOption(const OptionSyntax options[], std::string argv) {
  std::string lowerCaseArg(argv.substr(0, 4));
  makeLowerCase(lowerCaseArg);
  for (const OptionSyntax *o = &options[0];; ++o) {
    if (o->id.empty())
      return 0;
    if (lowerCaseArg == o->id.substr(0, 4))
      return o;
  }
}

Argument::Argument(int _position, const OptionSyntax &_key,
                   const std::string &_value)
    : position(_position), key(&_key), value(_value) {};

int Argument::intValue(const int min, const int max) const {
  int x = strtoistop(value);
  if (x < min || x > max)
    Complaint(usageError) << "unexpected value \"" + value + "\" for \""
                          << key->id << "\" option.\n"
                          << "Expected value between " << min << " and " << max
                          << ".";
  return x;
}

DOUBLE Argument::floatValue(const DOUBLE min, const DOUBLE max) const {
  DOUBLE x = strtofstop(value);
  if (x < min || x > max)
    Complaint(usageError) << "unexpected value \"" + value + "\" for \""
                          << key->id << "\" option.\n"
                          << "Expected value between " << min << " and " << max
                          << ".";
  return x;
}

bool Argument::operator<(const Argument &a) const {
  if (key->rank < a.key->rank)
    return true;
  if (a.key->rank < key->rank)
    return false;
  return (position < a.position);
}

Options options;

/*********************************************************
 * Canvas                                                 *
 *********************************************************/

Canvas::Canvas(const int _nrRows, const int _nrCols)
    : nrRows(_nrRows + strokeWidth[nrPens - 1]),
      nrCols(_nrCols + strokeWidth[nrPens - 1]),
      pixels(new RGB[nrRows * nrCols]), rows(new RGB *[nrRows]) {
  // let rows[i] point to first pixel in output colum in row i
  rows[0] = pixels + strokeWidth[nrPens - 1] / 2;
  for (int i = 1; i < nrRows; ++i)
    rows[i] = rows[i - 1] + nrCols;

  // reindex rows so that 0 is the first output row
  rows += strokeWidth[nrPens - 1] / 2;

  // calculate upper bound on total pen size
  int totalPenSize = 0;
  for (int i = 0; i < nrPens; ++i)
    totalPenSize += strokeWidth[i] * strokeWidth[i];

  // reserve space for pen masks
  int *penMask = new int[totalPenSize];

  // calculate pen masks
  for (int i = 0; i < nrPens; ++i) {
    penSize[i] = 0;
    penMasks[i] = penMask;
    int radius = strokeWidth[i] / 2;
    DOUBLE sqRadius = sqr(radius);
    for (int row = -radius; row < radius; ++row)
      for (int col = -radius; col < radius; ++col)
        if (sqr(0.5 + (DOUBLE)row) + sqr(0.5 + (DOUBLE)col) <= sqRadius) {
          *(penMask++) = row * nrCols + col;
          ++penSize[i];
        }
  }
}

const RGB &Canvas::operator()(const int row, const int col) const {
  return rows[row][col];
}

template <class T>
void Canvas::shade(const int row, const int col, const T &value) {
  rows[row][col] *= value;
}

void Canvas::draw(const int row, const int col, const int pen,
                  const RGB &colour) {
  if (row < 0 || row >= options.width || col < 0 || col > options.width)
    return;
  RGB *centre = rows[row] + col;
  int *penMask = penMasks[pen];
  for (int i = penSize[pen] - 1; i >= 0; --i)
    centre[penMask[i]] = colour;
}

template <class T>
void Canvas::shade(const int row, const int col, const int pen,
                   const T &value) {
  if (row < 0 || row >= options.width || col < 0 || col > options.width)
    return;
  RGB *centre = rows[row] + col;
  int *penMask = penMasks[pen];
  for (int i = penSize[pen] - 1; i >= 0; --i)
    centre[penMask[i]] *= value;
}

void Canvas::draw(const Point &p, const Point &q, const int pen,
                  const RGB &colour) {
  // calculate pixel coordinates
  Point pp((p.x - options.windowOffset) * options.pixelsPerUnit + 0.5,
           (p.y - options.windowOffset) * options.pixelsPerUnit + 0.5);
  Point qq((q.x - options.windowOffset) * options.pixelsPerUnit + 0.5,
           (q.y - options.windowOffset) * options.pixelsPerUnit + 0.5);

  Point vector = qq - pp;
  if (vector.inOrigin()) {
    draw((int)pp.y, (int)pp.x, pen, colour);
  }
  if (abs(vector.x) > abs(vector.y)) {
    int col = pp.x;
    int dCol = (vector.x < 0 ? -1 : 1);
    int tCol = qq.x;
    DOUBLE dRow = vector.ySlope() * dCol;
    DOUBLE row = pp.y - (pp.x - col) * vector.ySlope();
    for (int j = abs(tCol - col); j >= 0; --j, col += dCol, row += dRow)
      draw((int)row, col, pen, colour);
  } else {
    int row = pp.y;
    int dRow = (vector.y < 0 ? -1 : 1);
    int tRow = qq.y;
    DOUBLE dCol = vector.xSlope() * dRow;
    DOUBLE col = pp.x - (pp.y - row) * vector.xSlope();
    for (int j = abs(tRow - row); j >= 0; --j, col += dCol, row += dRow)
      draw(row, (int)col, pen, colour);
  }
}

/*********************************************************
 * Sites                                                  *
 *********************************************************/

Site::Site()
    : groupId(-1), siteId(-1), multiplicativeWeight(1), subtractiveWeight(0) {};

Site &Site::setId(int _groupId, int _siteId) {
  groupId = _groupId;
  siteId = _siteId;
  return *this;
}

void Site::verifyWeight(const DOUBLE weight) {
  static bool zeroWeightWarningGiven = false;
  if (options.weighting == Options::divisive) {
    if (weight == 0) {
      if (!zeroWeightWarningGiven) {
        Complaint(warning) << "sites with zero weight have empty regions";
        zeroWeightWarningGiven = true;
      }
    } else if (weight < 0)
      Complaint(userError) << "negative weight detected in input";
  }
}

Site &Site::setDivisiveWeight(DOUBLE weight) {
  multiplicativeWeight = (DOUBLE)1.0 / weight;
  return *this;
}

Site &Site::setSubtractiveWeight(DOUBLE weight) {
  subtractiveWeight = weight;
  return *this;
}

Site &Site::setWeight(DOUBLE weight) {
  if (options.weighting == Options::divisive)
    setDivisiveWeight(weight);
  else
    setSubtractiveWeight(weight);
  return *this;
}

Site &Site::copyWeight(const Site *s) {
  multiplicativeWeight = s->multiplicativeWeight;
  subtractiveWeight = s->subtractiveWeight;
  return *this;
}

DOUBLE Site::divisiveWeight() const {
  return (DOUBLE)1.0 / multiplicativeWeight;
}

void Site::weighDistance(DOUBLE &distance) const {
  switch (options.weighting) {
  case Options::subtractive:
    distance -= subtractiveWeight;
    return;
  case Options::divisive:
    if (distance == 0 && multiplicativeWeight == INF)
      // return INF, not NAN, as this would result in
      // empty ranges (ranges with invalid lower bounds)
      distance = INF;
    else
      distance *= multiplicativeWeight;
    return;
  }

  /* ALTERNATIVELY?
    if (multiplicativeWeight == 1) return;
    if (distance == 0) return;
    distance *= multiplicativeWeight;
  */
}

void Site::weighDistance(Range &range) const {
  weighDistance(range.lowerBound);
  weighDistance(range.upperBound);
}

DOUBLE Site::distance(const Distance *measure, const Point &p) const {
  return measure->distanceToSite(p, this);
}

void Site::boundDistance(const Distance *measure, const Point &centre,
                         DOUBLE halfwidth, Range &range) const {
  return measure->boundDistanceToSite(centre, halfwidth, this, range);
}

void Site::drawOutline(Canvas &canvas, const RGB &colour) const {
  Complaint(internalError) << "calling pure virtual method Site::drawOutline";
}

void Site::drawFill(Canvas &canvas, const RGB &colour) const {
  Complaint(internalError) << "calling pure virtual method Site::drawFill";
}

PointSite::PointSite(const Point &_location)
    : location(_location), distanceFromOrigin(_location.length()),
      directionFromOrigin(_location.normalised()) {};

PointSite *PointSite::random() {
  // dummy use of rnd to get same output as from version 0.25:
  long forget = rand();

  PointSite *p =
      new PointSite(Point(2.0 * rnd(-0.5, 0.5), 2.0 * rnd(-0.5, 0.5)));

  forget = rand();
  return p;
}

DOUBLE PointSite::distance(const Distance *measure, const Point &p) const {
  return measure->distanceToPointSite(p, this);
}

void PointSite::boundDistance(const Distance *measure, const Point &centre,
                              DOUBLE halfwidth, Range &range) const {
  return measure->boundDistanceToPointSite(centre, halfwidth, this, range);
}

void PointSite::drawOutline(Canvas &canvas, const RGB &colour) const {
  canvas.draw(location, location,
              (options.colours.sites.size() > 1 ? siteOutlinePen : sitePen),
              colour);
}

void PointSite::drawFill(Canvas &canvas, const RGB &colour) const {
  canvas.draw(location, location, sitePen, colour);
}

SphericalPointSite::SphericalPointSite(const SphericalPoint &_sphericalLocation)
    : sphericalLocation(_sphericalLocation) {};

SphericalPointSite *SphericalPointSite::random() {
  SphericalPoint location;
  location.z = rnd(-1, 1);
  DOUBLE scale = sqrt((DOUBLE)1 - sqr(location.z));
  DOUBLE longitude = rnd(-PI, PI);
  location.x = scale * cos(longitude);
  location.y = scale * sin(longitude);
  return new SphericalPointSite(location);
}

DOUBLE SphericalPointSite::distance(const Distance *measure,
                                    const Point &p) const {
  return measure->distanceToSphericalPointSite(p, this);
}

void SphericalPointSite::boundDistance(const Distance *measure,
                                       const Point &centre, DOUBLE halfwidth,
                                       Range &range) const {
  return measure->boundDistanceToSphericalPointSite(centre, halfwidth, this,
                                                    range);
}

void SphericalPointSite::drawOutline(Canvas &canvas, const RGB &colour) const {
  // rendering of spherical point sites not implemented
};

void SphericalPointSite::drawFill(Canvas &canvas, const RGB &colour) const {
  // rendering of spherical point sites not implemented
};

HyperbolicPointSite::HyperbolicPointSite(const GansPoint &_hyperbolicLocation)
    : hyperbolicLocation(_hyperbolicLocation) {};

HyperbolicPointSite *HyperbolicPointSite::random() {
  // unit disks by projection:
  // equal-area: radius 1
  // Gans: radius sinh(1) = (e^2 - 1)/(2e) ~ 1.175
  // Poincare disk: radius (e-1)/(e+1) ~ 0.462
  // Klein disk: radius sqrt((e^2-1)/(e^2+1)) ~ 0.873
  // Poincare halfplane (shifted down by 1):
  //   radius sinh(1) ~ 1.175, centre (0, cosh(1)-1) ~ (0, 0.543)

  // from equal-area to Gans: multiply by sinh(||p||)/||p||

  Point location;
  do {
    location.x = rnd(-1, 1);
    location.y = rnd(-1, 1);
  } while (location.sqr() > 1);
  DOUBLE l = location.length();
  GansPoint hyperbolicLocation;
  hyperbolicLocation = location * (sinh(l) / l);
  return new HyperbolicPointSite(hyperbolicLocation);
}

DOUBLE HyperbolicPointSite::distance(const Distance *measure,
                                     const Point &p) const {
  return measure->distanceToHyperbolicPointSite(p, this);
}

void HyperbolicPointSite::boundDistance(const Distance *measure,
                                        const Point &centre, DOUBLE halfwidth,
                                        Range &range) const {
  return measure->boundDistanceToHyperbolicPointSite(centre, halfwidth, this,
                                                     range);
}

void HyperbolicPointSite::drawOutline(Canvas &canvas, const RGB &colour) const {
  // rendering of hyperbolic point sites not implemented
};

void HyperbolicPointSite::drawFill(Canvas &canvas, const RGB &colour) const {
  // rendering of hyperbolic point sites not implemented
};

BigSite::BigSite(const DOUBLE _length)
    : length(_length), invLength((DOUBLE)1.0 / _length) {};

DOUBLE BigSite::distance(const Distance *measure, const Point &p) const {
  return measure->distanceToBigSite(p, this);
}

void BigSite::boundDistance(const Distance *measure, const Point &centre,
                            DOUBLE halfwidth, Range &range) const {
  return measure->boundDistanceToBigSite(centre, halfwidth, this, range);
}

DirectedPointSite::DirectedPointSite(const Point &_location,
                                     const Point &_vector)
    : PointSite(_location), direction(_vector.normalised()),
      normal(direction.rotLeft()) {};

DirectedPointSite *DirectedPointSite::random() {
  DOUBLE t = rnd(0, TWOPI);
  return new DirectedPointSite(
      Point(2.0 * rnd(-0.5, 0.5), 2.0 * rnd(-0.5, 0.5)), Point(cos(t), sin(t)));
}

DOUBLE DirectedPointSite::distance(const Distance *measure,
                                   const Point &p) const {
  return measure->distanceToDirectedPointSite(p, this);
}

void DirectedPointSite::boundDistance(const Distance *measure,
                                      const Point &centre, DOUBLE halfwidth,
                                      Range &range) const {
  return measure->boundDistanceToDirectedPointSite(centre, halfwidth, this,
                                                   range);
}

void DirectedPointSite::drawOutline(Canvas &canvas, const RGB &colour) const {
  PointSite::drawOutline(canvas, colour);
  Point arrowHead(location + direction * options.arrowLength);
  canvas.draw(location, arrowHead, arrowPen, colour);
  canvas.draw(arrowHead, arrowHead, arrowHeadPen, colour);
}

RootedVectorSite::RootedVectorSite(const Point &_location, const Point &_vector)
    : PointSite(_location), DirectedPointSite(_location, _vector),
      BigSite(_vector.length()) {};

RootedVectorSite *RootedVectorSite::random(const DOUBLE scale) {
  DOUBLE length = rnd(0.001, scale);
  // generate the following two numbers first, to obtain same output as
  // from version 0.25:
  DOUBLE rnd1 = rnd(-0.5, 0.5);
  DOUBLE rnd2 = rnd(-0.5, 0.5);
  DOUBLE t = rnd(0, TWOPI);
  return new RootedVectorSite(
      Point((2.0 - length) * rnd1, (2.0 - length) * rnd2),
      Point(cos(t) * length, sin(t) * length));
}

DOUBLE RootedVectorSite::distance(const Distance *measure,
                                  const Point &p) const {
  return measure->distanceToRootedVectorSite(p, this);
}

void RootedVectorSite::boundDistance(const Distance *measure,
                                     const Point &centre, DOUBLE halfwidth,
                                     Range &range) const {
  return measure->boundDistanceToRootedVectorSite(centre, halfwidth, this,
                                                  range);
}

void RootedVectorSite::drawOutline(Canvas &canvas, const RGB &colour) const {
  PointSite::drawOutline(canvas, colour);
  Point arrowHead(location + direction * length);
  canvas.draw(location, arrowHead, arrowPen, colour);
  canvas.draw(arrowHead, arrowHead, arrowHeadPen, colour);
}

int PolylineSite::size() const { return vertices.size(); }

const Point &PolylineSite::operator[](const int i) const {
  // assert(i >= 0 && i < size());
  return vertices[i];
}

PolylineSite::PolylineSite(const std::vector<Point> &points)
    : vertices(points), bboxMin(min(points)), bboxMax(max(points)) {};

PolylineSite *PolylineSite::random() {
  Point endpoint[2];
  DOUBLE t = rnd(0, TWOPI);
  for (int i = 0; i < 2; ++i) {
    DOUBLE dt = rnd(1.5, PI);
    t += dt;
    DOUBLE maxsqr = 0.98 / std::max(sqr(cos(t)), sqr(sin(t)));
    DOUBLE r = sqrt(rnd(0.5, maxsqr));
    endpoint[i] = Point(r * cos(t), r * sin(t));
  }
  Point travel = endpoint[1] - endpoint[0];
  int nrControlPoints = 2 + travel.length() / 0.3;
  Point wiggle = travel.rotLeft() * 0.15;

  Point controlPoint[nrControlPoints];
  controlPoint[0] = endpoint[0];
  for (int i = 1; i < nrControlPoints - 1; ++i)
    controlPoint[i] = endpoint[0] * ((DOUBLE)(nrControlPoints - 1 - i) /
                                     (nrControlPoints - 1)) +
                      endpoint[1] * ((DOUBLE)i / (nrControlPoints - 1)) +
                      wiggle * rnd(-0.5, 0.5);
  controlPoint[nrControlPoints - 1] = endpoint[1];

  int size = 2 * nrControlPoints - 2;
  std::vector<Point> points;
  points.push_back(endpoint[0]);
  for (int i = 1; i < nrControlPoints - 1; ++i) {
    points.push_back(controlPoint[i - 1] * 0.3 + controlPoint[i] * 0.7);
    points.push_back(controlPoint[i] * 0.7 + controlPoint[i + 1] * 0.3);
  }
  points.push_back(endpoint[1]);
  PolylineSite *s = new PolylineSite(points);
  return s;
}

DOUBLE PolylineSite::distance(const Distance *measure, const Point &p) const {
  return measure->distanceToPolylineSite(p, this);
}

void PolylineSite::boundDistance(const Distance *measure, const Point &centre,
                                 DOUBLE halfwidth, Range &range) const {
  return measure->boundDistanceToPolylineSite(centre, halfwidth, this, range);
}

void PolylineSite::drawOutline(Canvas &canvas, const RGB &colour) const {
  for (int i = 1; i < size(); ++i)
    canvas.draw(vertices[i - 1], vertices[i],
                (options.colours.sites.size() > 1 ? siteOutlinePen : sitePen),
                colour);
}

void PolylineSite::drawFill(Canvas &canvas, const RGB &colour) const {
  for (int i = 1; i < size(); ++i)
    canvas.draw(vertices[i - 1], vertices[i], sitePen, colour);
}

SegmentSite::SegmentSite(const Point &_origin, const Point &_destination)
    : PolylineSite(vectorOfTwo<Point>(_origin, _destination)),
      origin(vertices[0]), destination(vertices[1]),
      direction((_destination - _origin).normalised()),
      normal(direction.rotLeft()), slope(direction.ySlope()),
      invSlope(direction.xSlope()), length((_destination - _origin).length()),
      invLength((DOUBLE)1.0 / length), tspTourLength(NAN) {}

SegmentSite *SegmentSite::random(const DOUBLE scale) {
  DOUBLE length = rnd(0, scale);
  Point origin((2.0 - length) * rnd(-0.5, 0.5),
               (2.0 - length) * rnd(-0.5, 0.5));
  DOUBLE t = rnd(0, TWOPI);
  Point destination = origin + Point(length * cos(t), length * sin(t));
  return new SegmentSite(origin, destination);
}

DOUBLE SegmentSite::distance(const Distance *measure, const Point &p) const {
  return measure->distanceToSegmentSite(p, this);
}

void SegmentSite::boundDistance(const Distance *measure, const Point &centre,
                                DOUBLE halfwidth, Range &range) const {
  return measure->boundDistanceToSegmentSite(centre, halfwidth, this, range);
}

/*********************************************************
 * Highway map                                            *
 *********************************************************/

ValueId::ValueId(DOUBLE _value, int _id) : value(_value), id(_id) {};

bool ValueId::operator<(const ValueId &vi) const {
  if (value < vi.value)
    return true;
  if (value > vi.value)
    return false;
  return (id < vi.id);
}

HighwaySection::HighwaySection(const Point &p0, const Point &p1, DOUBLE _speed)
    : speed(_speed), cotanOfEntry(1.0L / sqrt(sqr(_speed) - 1)) {
  p[0] = p0;
  p[1] = p1;
  Point travel = p[1] - p[0];
  DOUBLE invSqLength = 1.0L / (travel * travel);
  invVector[0] = travel * invSqLength;
  invVector[1] = travel.rotLeft() * invSqLength;
  invWidth = (DOUBLE)1.0 / travel.x;
  invHeight = (DOUBLE)1.0 / travel.y;
  orientation = speed * (abs(travel.y) - abs(travel.x)) / travel.length();
}

void HighwaySection::calculateInterchange(HighwaySection &other, DOUBLE &tSelf,
                                          DOUBLE &tOther) {
  tSelf = INF;
  tOther = INF;
  Point otherTranslated = other.p[0] - p[0];
  Point other0relative(invVector[0] * otherTranslated,
                       invVector[1] * otherTranslated);
  otherTranslated = other.p[1] - p[0];
  Point other1relative(invVector[0] * otherTranslated,
                       invVector[1] * otherTranslated);
  if (other0relative.y == other1relative.y)
    return;
  tOther = other0relative.y / (other0relative.y - other1relative.y);
  tSelf = other0relative.x * (-tOther + 1.0) + other1relative.x * tOther;
}

Point HighwaySection::rampLocation(const DOUBLE t) const {
  return p[0] * (-t + 1.0) + p[1] * t;
}

void HighwaySection::drawOutline(Canvas &canvas, const RGB &colour) const {
  canvas.draw(p[0], p[1], sitePen, colour);
}

void HighwaySection::drawFill(Canvas &canvas, const RGB &colour) const {
  canvas.draw(p[0], p[1], bisectorPen, colour);
}

template <class E>
HighwayMap<E>::HighwayNode::HighwayNode(const Point &_location,
                                        const int nrSites)
    : location(_location), distances(new DOUBLE[nrSites]) {
  for (int i = 0; i < nrSites; ++i)
    distances[i] = INF;
}

template <class E>
void HighwayMap<E>::HighwayNode::connect(int nodeId, DOUBLE distance) {
  neighbours.push_back(ValueId(distance, nodeId));
}

template <class E>
HighwayMap<E>::Probe::Probe(DOUBLE _distance, int _nodeId, int _siteId)
    : distance(_distance), nodeId(_nodeId), siteId(_siteId){};

template <class E>
bool HighwayMap<E>::Probe::operator<(const HighwayMap<E>::Probe &p) const {
  return (distance > p.distance);
}

// O3

// distance via node
template <class E>
DOUBLE HighwayMap<E>::distance(const Point &p, HighwayNode &n, int siteId) {
  // O4
  return (E::crossCountryDistance(p, n.location) + n.distances[siteId]);
}

// distance via highway section
template <class E>
DOUBLE HighwayMap<E>::distance(const Point &p, HighwaySection &hw, int siteId) {
  DOUBLE d = INF;
  DOUBLE t[2];
  E::calculateRamps(hw, t, p);
  // the next ten lines do not seem to be needed for correctness, but speed up
  // the entire computation by a factor three!
  if (t[0] <= 0)
    keepMin(d, E::crossCountryDistance(p, hw.p[0]) +
                   nodes[hw.ramps.front().id].distances[siteId]);
  if (t[1] >= 1)
    keepMin(d, E::crossCountryDistance(p, hw.p[1]) +
                   nodes[hw.ramps.back().id].distances[siteId]);
  for (int i = 0; i < 2; ++i) {
    if (!(t[i] > 0 && t[i] < 1))
      continue;
    Point rampLocation = hw.rampLocation(t[i]);

    int j = 0;
    // find the first node in direction i; will simple linear search do?
    // while (hw.ramps[j].value < t[i]) ++j; // safe bcs last ramp is at 1 >=
    // t[i];

    // alternatively: use binary search:
    int maxj = hw.ramps.size() - 1; // size() >= 2, so maxj >= 1
    while (j < maxj - 1)            // so maxj >= j+2
    {
      int midj = (j + maxj) / 2;
      if (hw.ramps[midj].value < t[i])
        j = midj;
      else
        maxj = midj;
    }
    // now ramps[j].value < t[i] <= ramps[j+1].value
    ++j;
    // end of binary search

    // now ramps[j-1].value < t[i] <= ramps[j].value
    HighwayNode &n = nodes[hw.ramps[j - 1 + i].id];
    keepMin(d, E::crossCountryDistance(p, rampLocation) +
                   (rampLocation - n.location).length() / hw.speed +
                   n.distances[siteId]);
  }
  return d;
}

template <class E>
HighwayMap<E>::HighwayMap(int nrGroups, std::vector<Site *> &sites,
                          std::vector<HighwaySection *> &_highways)
    : gridWidth((options.width - 1) / cellWidth + 1),
      cellsPerUnit((DOUBLE)options.width / options.windowWidth / cellWidth),
      highways(_highways) {
  int nrSites = sites.size();

  int nrHighways = highways.size();
  long nrEdges = 0;

  std::priority_queue<Probe> queue;
  // create nodes for the endpoints of highway sections;
  Point lastNode(NAN, NAN);
  for (int i = 0; i < nrHighways; ++i) {
    // O4
    if (lastNode != highways[i]->p[0])
      nodes.push_back(HighwayNode(highways[i]->p[0], nrSites));
    highways[i]->ramps.push_back(ValueId(0, nodes.size() - 1));
    nodes.push_back(HighwayNode(highways[i]->p[1], nrSites));
    highways[i]->ramps.push_back(ValueId(1, nodes.size() - 1));
    lastNode = highways[i]->p[1];
  }

  // connect them to the other sections' endpoints
  // (the following code also makes useless connections between the
  // end points of the same section, but the effect on the running time
  // should be negligible)
  int nrHighwayEndpoints = nodes.size();
  for (int g = 1; g < nrHighwayEndpoints; ++g)
    for (int h = 0; h < g; ++h) {
      DOUBLE distance =
          E::crossCountryDistance(nodes[h].location, nodes[g].location);
      nodes[h].connect(g, distance);
      ++nrEdges;
      nodes[g].connect(h, distance);
      ++nrEdges;
    }

  // create vertices for the sites, connect them to the section
  // endpoints, and insert them in the queue for the SP computation
  for (int s = 0; s < nrSites; ++s) {
    queue.push(Probe(0, nrHighwayEndpoints + s, sites[s]->siteId));
    nodes.push_back(
        HighwayNode(dynamic_cast<PointSite *>(sites[s])->location, nrSites));
    for (int g = 0; g < nrHighwayEndpoints; ++g)
      nodes.back().connect(
          g, E::crossCountryDistance(nodes.back().location, nodes[g].location));
    nrEdges += nrHighwayEndpoints;
  }
  nrPrimaryNodes = nodes.size();

  // create vertices for all highway intersections
  for (int h = 0; h < highways.size(); ++h) {
    HighwaySection &hs = *highways[h];
    for (int g = 0; g < h; ++g) {
      HighwaySection &gs = *highways[g];
      DOUBLE gt, ht;
      gs.calculateInterchange(hs, gt, ht);
      if (ht <= 0 || ht >= 1 || gt <= 0 || gt >= 1)
        continue;
      int interchangeId = nodes.size();
      Point interchangeLocation = gs.rampLocation(gt);
      nodes.push_back(HighwayNode(interchangeLocation, nrSites));
      gs.ramps.push_back(ValueId(gt, interchangeId));
      hs.ramps.push_back(ValueId(ht, interchangeId));
    }
  }
  int nrInputNodes = nodes.size();

  // create vertices for all points where a ray from one of the
  // sites or section endpoints reaches another highway section
  // under angle arccos(1/speed).
  for (int h = 0; h < highways.size(); ++h) {
    if (options.verbose)
      reportProgress(((DOUBLE)h) / highways.size(), "> Building highway map");

    HighwaySection &hw = *highways[h];
    for (int v = 0; v < nrPrimaryNodes; ++v) {
      if (h == v / 2)
        continue; // v is one of the endpoints of hw
      DOUBLE t[2];
      E::calculateRamps(hw, t, nodes[v].location);
      for (int i = 0; i < 2; ++i) {
        if (!(t[i] > 0 && t[i] < 1))
          continue;
        Point rampLocation = hw.rampLocation(t[i]);
        DOUBLE distance =
            E::crossCountryDistance(rampLocation, nodes[v].location);
        // O6
        int rampNodeId = nodes.size();
        nodes.push_back(HighwayNode(rampLocation, nrSites));
        nodes[v].connect(rampNodeId, distance);
        ++nrEdges;
        nodes[rampNodeId].connect(v, distance);
        ++nrEdges;
        hw.ramps.push_back(ValueId(t[i], rampNodeId));
      }
    }
    sort(hw.ramps.begin(), hw.ramps.end()); // by t-value (first of Ramp)
    for (int i = 1; i < hw.ramps.size(); ++i) {
      DOUBLE distance =
          // Euclidean distance here, also for Manhattan highways!
          (nodes[hw.ramps[i].id].location - nodes[hw.ramps[i - 1].id].location)
              .length() /
          hw.speed;
      nodes[hw.ramps[i - 1].id].connect(hw.ramps[i].id, distance);
      ++nrEdges;
      nodes[hw.ramps[i].id].connect(hw.ramps[i - 1].id, distance);
      ++nrEdges;
    }
  }
  if (options.verbose)
    reportProgress(1, "> Building highway map");

  // use Dijkstra's algorithm to compute distances from each node to each site
  // (not implemented: under some circumstances, this could be sped up by
  // stopping the algorithm as soon as the closest two sites to each node are
  // known---but not with farthest-site and/or divisively weighted VD)

  nrEdges *= nrSites;
  long nrPopped = 0;
  while (!queue.empty()) {
    if (options.verbose && (nrPopped & 0xFFFF) == 0)
      reportProgress(((DOUBLE)nrPopped) / nrEdges,
                     "> Computing shortest paths");
    Probe p = queue.top();
    queue.pop();
    ++nrPopped;
    HighwayNode &n = nodes[p.nodeId];
    if (p.distance >= n.distances[p.siteId])
      continue;
    n.distances[p.siteId] = p.distance;
    for (int i = 0; i < n.neighbours.size(); ++i)
      queue.push(Probe(p.distance + n.neighbours[i].value, n.neighbours[i].id,
                       p.siteId));
  }
  if (options.verbose)
    reportProgress(1, "> Computing shortest paths");

  cells = newMatrix<Cell>(nrSites, gridWidth, gridWidth);
  int totalRegisteredNodes = 0;
  int totalRegisteredSections = 0;
  const DOUBLE cellHalfWidth =
      options.windowWidth / options.width * cellWidth / 2;
  const DOUBLE cellHalfDiameterCrossCountry = // with error margin:
      options.windowWidth / options.width * (cellWidth + 3) *
      E::crossCountryDistance(Point(0, 0), Point(0.5, 0.5));

  DOUBLE *nodeDistance[nrSites];
  for (int i = 0; i < nrSites; ++i)
    nodeDistance[i] = new DOUBLE[nrPrimaryNodes];
  DOUBLE *sectionDistance[nrSites];
  for (int i = 0; i < nrSites; ++i)
    sectionDistance[i] = new DOUBLE[highways.size()];
  DOUBLE siteUpperBound[nrSites];
  DOUBLE groupUpperBound[nrGroups];

  for (int gridRow = 0; gridRow < gridWidth; ++gridRow) {
    if (options.verbose)
      reportProgress(((DOUBLE)gridRow) / gridWidth,
                     "> Computing best access points");

    for (int gridCol = 0; gridCol < gridWidth; ++gridCol) {
      Point cellCentre(
          (2.0 * gridCol + 1.0) * cellHalfWidth + options.windowOffset,
          (2.0 * gridRow + 1.0) * cellHalfWidth + options.windowOffset);
      for (int groupId = 0; groupId < nrGroups; ++groupId)
        groupUpperBound[groupId] = INF;
      for (int siteId = 0; siteId < nrSites; ++siteId) {
        siteUpperBound[siteId] = INF;
        for (int i = 0; i < nrPrimaryNodes; ++i) {
          DOUBLE d = distance(cellCentre, nodes[i], siteId);
          nodeDistance[siteId][i] = d;
          keepMin(siteUpperBound[siteId], d);
        }
        for (int h = 0; h < highways.size(); ++h) {
          DOUBLE d = distance(cellCentre, *highways[h], siteId);
          sectionDistance[siteId][h] = d;
          keepMin(siteUpperBound[siteId], d);
        }
        siteUpperBound[siteId] += cellHalfDiameterCrossCountry;
        sites[siteId]->weighDistance(siteUpperBound[siteId]);
        keepMin(groupUpperBound[sites[siteId]->groupId],
                siteUpperBound[siteId]);
      }
      DOUBLE firstUpperBound = INF;
      DOUBLE secondUpperBound = INF;
      for (int groupId = 0; groupId < nrGroups; ++groupId) {
        keepMin(secondUpperBound, groupUpperBound[groupId]);
        if (secondUpperBound < firstUpperBound)
          std::swap(firstUpperBound, secondUpperBound);
      }
      DOUBLE mapUpperBound = INF;

      if (!options.useCompositeDistance) {
        switch (options.order) {
        case Options::first:
          mapUpperBound = firstUpperBound + options.equalityThreshold;
          break;
        case Options::second:
        case Options::nextclosest:
          mapUpperBound = secondUpperBound + options.equalityThreshold;
          break;
        case Options::farthest:
          mapUpperBound = INF;
          break;
        default:
          Complaint(internalError)
              << "unknown diagram order in HighwayMap constructor";
        }
      }

      for (int siteId = 0; siteId < nrSites; ++siteId) {
        Cell &cell = cells[siteId][gridRow][gridCol];
        DOUBLE cellUpperBound = std::min(mapUpperBound, siteUpperBound[siteId]);

        // distance(cellCentre, s) + cellDiameter/2 is an upper bound
        // for routes via n,
        //   distance from centre via n - cellDiameter/2 is a lower bound,
        //   so need to be considered only if
        //   distance from centre via n - cellDiameter/2 <
        //     distance(cellCentre, s) + cellDiameter/2         <=>

        for (int i = 0; i < nrPrimaryNodes; ++i) {
          DOUBLE lowerBound =
              std::max((DOUBLE)0,
                       nodeDistance[siteId][i] - cellHalfDiameterCrossCountry);
          sites[siteId]->weighDistance(lowerBound);
          if (lowerBound > cellUpperBound)
            continue;
          cell.nodes.push_back(i);
          ++totalRegisteredNodes;
        }

        for (int h = 0; h < highways.size(); ++h) {
          DOUBLE lowerBound =
              std::max((DOUBLE)0, sectionDistance[siteId][h] -
                                      cellHalfDiameterCrossCountry);
          sites[siteId]->weighDistance(lowerBound);
          if (lowerBound > cellUpperBound)
            continue;
          cell.sections.push_back(h);
          ++totalRegisteredSections;
        }
      }
    }
  }
  if (options.verbose)
    reportProgress(1, "> Computing best access points");

  for (int i = 0; i < nrSites; ++i)
    delete[] nodeDistance[i];
  for (int i = 0; i < nrSites; ++i)
    delete[] sectionDistance[i];

  /*
    if (options.verbose)
    {
      std::cerr << "\r"
           << "Highway map grid size: "
           << gridWidth << " x " << gridWidth << " cells" << std::endl
           << "Average number of registered nodes per site per cell: "
           << (DOUBLE) totalRegisteredNodes / (nrGroups * gridWidth * gridWidth)
    << std::endl
           << "Average number of registered highway sections per site per cell:
    "
           << (DOUBLE) totalRegisteredSections / (nrGroups * gridWidth *
    gridWidth) << std::endl
           << "Preprocessing input...";
      std::cerr.flush();
    }
  */
}

template <class E>
DOUBLE HighwayMap<E>::distance(const Point &p, const int siteId) {
  int gridRow = (p.y - options.windowOffset) * cellsPerUnit;
  if (gridRow < 0)
    gridRow = 0;
  else if (gridRow >= gridWidth)
    gridRow = gridWidth - 1;
  int gridCol = (p.x - options.windowOffset) * cellsPerUnit;
  if (gridCol < 0)
    gridCol = 0;
  else if (gridCol >= gridWidth)
    gridCol = gridWidth - 1;

  Cell &cell = cells[siteId][gridRow][gridCol];
  DOUBLE d = INF;

  for (std::vector<int>::iterator i = cell.nodes.begin(); i != cell.nodes.end();
       ++i)
    keepMin(d, distance(p, nodes[*i], siteId));

  for (std::vector<int>::iterator i = cell.sections.begin();
       i != cell.sections.end(); ++i)
    keepMin(d, distance(p, *highways[*i], siteId));

  return d;
}

template <class E> void HighwayMap<E>::showNodes(Canvas &canvas) {
  for (int row = 1; row < gridWidth; ++row) {
    Point p(options.windowWidth * row / gridWidth + options.windowOffset,
            options.windowOffset);
    Point q(options.windowWidth * row / gridWidth + options.windowOffset,
            options.windowOffset + options.windowWidth);
    canvas.draw(p, q, contourPen, options.colours.grid);
  }
  for (int col = 1; col < gridWidth; ++col) {
    Point p(options.windowOffset,
            options.windowWidth * col / gridWidth + options.windowOffset);
    Point q(options.windowOffset + options.windowWidth,
            options.windowWidth * col / gridWidth + options.windowOffset);
    canvas.draw(p, q, contourPen, options.colours.grid);
  }
  for (int i = 0; i < nodes.size(); ++i) {
    Point pixel(
        (nodes[i].location.x - options.windowOffset) * options.pixelsPerUnit +
            0.5,
        (nodes[i].location.y - options.windowOffset) * options.pixelsPerUnit +
            0.5);
    canvas.draw(pixel.y, pixel.x, bisectorPen, options.colours.node);
  }
}

DOUBLE EuclideanHighwayEnvironment::crossCountryDistance(const Point &p,
                                                         const Point &q) {
  return (p - q).length();
}

void EuclideanHighwayEnvironment::calculateRamps(const HighwaySection &hw,
                                                 DOUBLE *t, const Point &q) {
  Point pq = q - hw.p[0];
  Point relativeLocation(hw.invVector[0] * pq, hw.invVector[1] * pq);
  t[0] = relativeLocation.x - abs(relativeLocation.y) * hw.cotanOfEntry;
  t[1] = relativeLocation.x + abs(relativeLocation.y) * hw.cotanOfEntry;
}

DOUBLE ManhattanHighwayEnvironment::crossCountryDistance(const Point &p,
                                                         const Point &q) {
  return abs(p.x - q.x) + abs(p.y - q.y);
}

void ManhattanHighwayEnvironment::calculateRamps(const HighwaySection &hw,
                                                 DOUBLE *t, const Point &q) {
  if (hw.orientation < -1)
    t[1] = t[0] = (q.x - hw.p[0].x) * hw.invWidth;
  else if (hw.orientation <= 1) {
    t[0] = (q.x - hw.p[0].x) * hw.invWidth;
    t[1] = (q.y - hw.p[0].y) * hw.invHeight;
    if (t[0] > t[1])
      std::swap(t[0], t[1]);
  } else
    t[0] = t[1] = (q.y - hw.p[0].y) * hw.invHeight;
}

/*********************************************************
 * Distance                                               *
 *********************************************************/

const std::string Distance::description() const {
  Complaint(internalError) << "virtual function Distance::description() called";
  return "abstract";
}

SiteTypes Distance::supportedSites() const {
  Complaint(internalError)
      << "virtual function Distance::supportedSites() called";
  return 0;
}

bool Distance::supports(const SiteTypes s) const {
  return (~supportedSites() & s) == 0;
}

DOUBLE Distance::distanceToSite(const Point &p, const Site *s) const {
  Complaint(internalError)
      << "virtual function Distance::distanceToSite() called";
  return NAN;
}

void Distance::boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                   const Site *s, Range &range) const {
  range.set(-INF, INF);
}

DOUBLE Distance::distanceToPointSite(const Point &p, const PointSite *s) const {
  return distanceToSite(p, s);
}

void Distance::boundDistanceToPointSite(const Point &centre, DOUBLE halfwidth,
                                        const PointSite *s,
                                        Range &range) const {
  boundDistanceToSite(centre, halfwidth, s, range);
}

DOUBLE
Distance::distanceToSphericalPointSite(const Point &p,
                                       const SphericalPointSite *s) const {
  return distanceToSite(p, s);
}

void Distance::boundDistanceToSphericalPointSite(const Point &centre,
                                                 DOUBLE halfwidth,
                                                 const SphericalPointSite *s,
                                                 Range &range) const {
  boundDistanceToSite(centre, halfwidth, s, range);
}

DOUBLE
Distance::distanceToHyperbolicPointSite(const Point &p,
                                        const HyperbolicPointSite *s) const {
  return distanceToSite(p, s);
}

void Distance::boundDistanceToHyperbolicPointSite(const Point &centre,
                                                  DOUBLE halfwidth,
                                                  const HyperbolicPointSite *s,
                                                  Range &range) const {
  boundDistanceToSite(centre, halfwidth, s, range);
}

DOUBLE Distance::distanceToDirectedPointSite(const Point &p,
                                             const DirectedPointSite *s) const {
  return distanceToPointSite(p, s);
}

void Distance::boundDistanceToDirectedPointSite(const Point &centre,
                                                DOUBLE halfwidth,
                                                const DirectedPointSite *s,
                                                Range &range) const {
  boundDistanceToPointSite(centre, halfwidth, s, range);
}

DOUBLE Distance::distanceToBigSite(const Point &p, const BigSite *s) const {
  return distanceToSite(p, s);
}

void Distance::boundDistanceToBigSite(const Point &centre, DOUBLE halfwidth,
                                      const BigSite *s, Range &range) const {
  boundDistanceToSite(centre, halfwidth, s, range);
}

DOUBLE Distance::distanceToRootedVectorSite(const Point &p,
                                            const RootedVectorSite *s) const {
  return distanceToPointSite(p, s);
}

void Distance::boundDistanceToRootedVectorSite(const Point &centre,
                                               DOUBLE halfwidth,
                                               const RootedVectorSite *s,
                                               Range &range) const {
  boundDistanceToPointSite(centre, halfwidth, s, range);
}

DOUBLE Distance::distanceToPolylineSite(const Point &p,
                                        const PolylineSite *s) const {
  return distanceToSite(p, s);
}

void Distance::boundDistanceToPolylineSite(const Point &centre,
                                           DOUBLE halfwidth,
                                           const PolylineSite *s,
                                           Range &range) const {
  boundDistanceToSite(centre, halfwidth, s, range);
}

DOUBLE Distance::distanceToSegmentSite(const Point &p,
                                       const SegmentSite *s) const {
  return distanceToPolylineSite(p, s);
}

void Distance::boundDistanceToSegmentSite(const Point &centre, DOUBLE halfwidth,
                                          const SegmentSite *s,
                                          Range &range) const {
  boundDistanceToPolylineSite(centre, halfwidth, s, range);
}

SiteTypes FieldDistance::supportedSites() const { return ALLTYPES; }

ConstantDistance::ConstantDistance(const DOUBLE _weight) : weight(_weight) {};

const std::string ConstantDistance::description() const {
  std::stringstream s;
  s << weight;
  return s.str();
}

DOUBLE ConstantDistance::distanceToSite(const Point &p, const Site *s) const {
  return weight;
}

void ConstantDistance::boundDistanceToSite(const Point &centre,
                                           DOUBLE halfwidth, const Site *s,
                                           Range &range) const {
  range.set(weight, weight);
}

const std::string FieldXDistance::description() const { return "fx"; }

DOUBLE FieldXDistance::distanceToSite(const Point &p, const Site *s) const {
  return p.x;
}

void FieldXDistance::boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                         const Site *s, Range &range) const {
  range.set(centre.x - halfwidth, centre.x + halfwidth);
}

const std::string FieldYDistance::description() const { return "fy"; }

DOUBLE FieldYDistance::distanceToSite(const Point &p, const Site *s) const {
  return p.y;
}

void FieldYDistance::boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                         const Site *s, Range &range) const {
  range.set(centre.y - halfwidth, centre.y + halfwidth);
}

const std::string FieldRDistance::description() const { return "fr"; }

DOUBLE FieldRDistance::distanceToSite(const Point &p, const Site *s) const {
  static Point lastPoint = Point(NAN, NAN);
  static DOUBLE lastLength;
  if (p != lastPoint) {
    lastPoint = p;
    lastLength = p.length();
  }
  return lastLength;
}

void FieldRDistance::boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                         const Site *s, Range &range) const {
  static Point lastCentre = Point(NAN, NAN);
  static DOUBLE lastHalfwidth;
  static Range lastRange;
  if (centre != lastCentre || halfwidth != lastHalfwidth) {
    bool onYAxis = nonDecreasing(centre.x - halfwidth, 0, centre.x + halfwidth);
    bool onXAxis = nonDecreasing(centre.y - halfwidth, 0, centre.y + halfwidth);
    if (onXAxis && onYAxis)
      lastRange.lowerBound = 0;
    else if (onXAxis)
      lastRange.lowerBound = abs(centre.x) - halfwidth;
    else if (onYAxis)
      lastRange.lowerBound = abs(centre.y) - halfwidth;
    else
      lastRange.lowerBound =
          sqrt(sqr(abs(centre.x) - halfwidth) + sqr(abs(centre.y) - halfwidth));
    lastRange.upperBound =
        sqrt(sqr(abs(centre.x) + halfwidth) + sqr(abs(centre.y) + halfwidth));
  }
  range = lastRange;
}

void SiteDistance::boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                       const Site *s, Range &range) const {
  range.set(s->distance(this, centre));
}

const std::string SiteXDistance::description() const { return "sx"; }

SiteTypes SiteXDistance::supportedSites() const { return POINTTYPE; }

DOUBLE SiteXDistance::distanceToPointSite(const Point &p,
                                          const PointSite *s) const {
  return s->location.x;
}

const std::string SiteYDistance::description() const { return "sy"; }

SiteTypes SiteYDistance::supportedSites() const { return POINTTYPE; }

DOUBLE SiteYDistance::distanceToPointSite(const Point &p,
                                          const PointSite *s) const {
  return s->location.y;
}

const std::string SiteRDistance::description() const { return "sr"; }

SiteTypes SiteRDistance::supportedSites() const { return POINTTYPE; }

DOUBLE SiteRDistance::distanceToPointSite(const Point &p,
                                          const PointSite *s) const {
  return s->distanceFromOrigin;
}

template <char S>
BinaryCompositionDistance<S>::BinaryCompositionDistance(const Distance *_a,
                                                        const Distance *_b)
    : a(_a), b(_b) {
  options.useCompositeDistance = true;
}

template <char S>
const std::string BinaryCompositionDistance<S>::description() const {
  std::stringstream s;
  s << '(' << a->description() << ' ' << S << ' ' << b->description() << ')';
  return s.str();
}

template <char S>
SiteTypes BinaryCompositionDistance<S>::supportedSites() const {
  return a->supportedSites() & b->supportedSites();
}

DOUBLE MinDistance::distanceToSite(const Point &p, const Site *s) const {
  DOUBLE valueA = s->distance(a, p);
  DOUBLE valueB = s->distance(b, p);
  if (isnan(valueA))
    return valueB;
  if (isnan(valueB))
    return valueA;
  return std::min(valueA, valueB);
}

void MinDistance::boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                      const Site *s, Range &range) const {
  Range valueA;
  s->boundDistance(a, centre, halfwidth, valueA);
  Range valueB;
  s->boundDistance(b, centre, halfwidth, valueB);
  range.lowerBound = safeMin(valueA.lowerBound, valueB.lowerBound);
  range.upperBound = safeMin(valueA.upperBound, valueB.upperBound);
}

DOUBLE MaxDistance::distanceToSite(const Point &p, const Site *s) const {
  DOUBLE valueA = s->distance(a, p);
  DOUBLE valueB = s->distance(b, p);
  if (isnan(valueA))
    return valueB;
  if (isnan(valueB))
    return valueA;
  return std::max(valueA, valueB);
}

void MaxDistance::boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                      const Site *s, Range &range) const {
  Range valueA;
  s->boundDistance(a, centre, halfwidth, valueA);
  Range valueB;
  s->boundDistance(b, centre, halfwidth, valueB);
  range.lowerBound = safeMax(valueA.lowerBound, valueB.lowerBound);
  range.upperBound = safeMax(valueA.upperBound, valueB.upperBound);
}

DOUBLE SumDistance::distanceToSite(const Point &p, const Site *s) const {
  return s->distance(a, p) + s->distance(b, p);
}

void SumDistance::boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                      const Site *s, Range &range) const {
  s->boundDistance(a, centre, halfwidth, range);
  Range valueB;
  s->boundDistance(b, centre, halfwidth, valueB);
  range += valueB;
}

DOUBLE DifferenceDistance::distanceToSite(const Point &p, const Site *s) const {
  return s->distance(a, p) - s->distance(b, p);
}

void DifferenceDistance::boundDistanceToSite(const Point &centre,
                                             DOUBLE halfwidth, const Site *s,
                                             Range &range) const {
  s->boundDistance(a, centre, halfwidth, range);
  Range valueB;
  s->boundDistance(b, centre, halfwidth, valueB);
  range -= valueB;
}

DOUBLE AbsoluteDifferenceDistance::distanceToSite(const Point &p,
                                                  const Site *s) const {
  return abs(s->distance(a, p) - s->distance(b, p));
}

void AbsoluteDifferenceDistance::boundDistanceToSite(const Point &centre,
                                                     DOUBLE halfwidth,
                                                     const Site *s,
                                                     Range &range) const {
  range.erase();

  Range diff;
  s->boundDistance(a, centre, halfwidth, diff);
  if (diff.isEmpty())
    return;
  Range valueB;
  s->boundDistance(b, centre, halfwidth, valueB);
  if (valueB.isEmpty())
    return;

  diff -= valueB;
  if (diff.contains(0))
    range.lowerBound = 0;
  else
    range.lowerBound = std::max(-diff.upperBound, diff.lowerBound);
  range.upperBound = std::max(-diff.lowerBound, diff.upperBound);
}

DOUBLE ProductDistance::distanceToSite(const Point &p, const Site *s) const {
  return s->distance(a, p) * s->distance(b, p);
}

void ProductDistance::boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                          const Site *s, Range &range) const {
  range.erase();

  Range valueA;
  s->boundDistance(a, centre, halfwidth, valueA);
  Range valueB;
  s->boundDistance(b, centre, halfwidth, valueB);
  if (valueA.isEmpty() || valueB.isEmpty())
    return;

  // if one of the ranges is [0,0] and the other is (-inf,inf), then
  // the product can be 0 (or invalid, nothing else):
  if ((valueA.is(0, 0) && valueB.is(-INF, INF)) ||
      (valueB.is(0, 0) && valueA.is(-INF, INF))) {
    range.set(0);
    return;
  }

  // otherwise extremes are obtained by multiplying an extreme of one
  // range with an extreme of the other; whether these are lower or
  // upper bounds depends on the signs
  range.extendTo(valueA.lowerBound * valueB.lowerBound);
  range.extendTo(valueA.lowerBound * valueB.upperBound);
  range.extendTo(valueA.upperBound * valueB.lowerBound);
  range.extendTo(valueA.upperBound * valueB.upperBound);
}

DOUBLE RatioDistance::distanceToSite(const Point &p, const Site *s) const {
  return s->distance(a, p) / s->distance(b, p);
}

void RatioDistance::boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                        const Site *s, Range &range) const {
  range.erase();

  Range valueA;
  s->boundDistance(a, centre, halfwidth, valueA);
  Range valueB;
  s->boundDistance(b, centre, halfwidth, valueB);
  if (valueA.isEmpty() || valueB.isEmpty())
    return;

  // consider the result of division by zero
  if (valueB.contains(0)) {
    if (valueA.lowerBound < 0)
      range.lowerBound = -INF;
    if (valueA.upperBound > 0)
      range.upperBound = INF;

    // consider the result of division by zero approached from below
    if (valueB.lowerBound < 0) {
      if (valueA.upperBound > 0)
        range.lowerBound = -INF;
      if (valueA.lowerBound < 0)
        range.upperBound = INF;
    }
  }

  // consider the result of dividing by arbitrarily large numbers
  if (valueA.lowerBound < INF && valueA.upperBound > -INF &&
      (valueB.lowerBound == -INF || valueB.upperBound == INF))
    range.extendTo(0);

  // otherwise extremes are obtained by dividing an extreme of one
  // range by an extreme of the other; whether these are lower or
  // upper bounds depends on the signs:
  range.extendTo(valueA.lowerBound / valueB.lowerBound);
  range.extendTo(valueA.lowerBound / valueB.upperBound);
  range.extendTo(valueA.upperBound / valueB.lowerBound);
  range.extendTo(valueA.upperBound / valueB.upperBound);
}

DOUBLE PowerDistance::distanceToSite(const Point &p, const Site *s) const {
  DOUBLE da = s->distance(a, p);
  if (da < 0)
    Complaint(userError) << "negative value " << da
                         << " for first operand to power operator detected";
  return pow(da, s->distance(b, p));
}

void PowerDistance::boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                        const Site *s, Range &range) const {
  range.erase();

  Range valueA;
  s->boundDistance(a, centre, halfwidth, valueA);
  Range valueB;
  s->boundDistance(b, centre, halfwidth, valueB);
  if (valueA.isEmpty() || valueB.isEmpty())
    return;

  // the first operand is not allowed to be negative:
  if (valueA.upperBound < 0)
    Complaint(userError)
        << "negative value " << valueA.upperBound
        << " for upper bound on first operand to power operator detected";
  if (valueA.lowerBound < 0)
    valueA.lowerBound = 0;

  // extremes are obtained by taking an extreme of the base range
  // to an extreme of the power range; whether these are lower or
  // upper bounds depends on where the base lies w.r.t. 1:
  range.extendTo(pow(valueA.lowerBound, valueB.lowerBound));
  range.extendTo(pow(valueA.lowerBound, valueB.upperBound));
  range.extendTo(pow(valueA.upperBound, valueB.lowerBound));
  range.extendTo(pow(valueA.upperBound, valueB.upperBound));
}

DOUBLE LessDistance::distanceToSite(const Point &p, const Site *s) const {
  DOUBLE valueA = s->distance(a, p);
  DOUBLE valueB = s->distance(b, p);
  return (valueA < valueB ? 1 : 0);
}

void LessDistance::boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                       const Site *s, Range &range) const {
  range.erase();

  Range valueA;
  s->boundDistance(a, centre, halfwidth, valueA);
  Range valueB;
  s->boundDistance(b, centre, halfwidth, valueB);
  if (valueA.isEmpty() || valueB.isEmpty())
    return;

  if (valueA.lowerBound < valueB.upperBound)
    range.extendTo(1);
  if (valueA.upperBound >= valueB.lowerBound)
    range.extendTo(0);
}

DOUBLE ConditionalDistance::distanceToSite(const Point &p,
                                           const Site *s) const {
  return (s->distance(a, p) != 0 ? s->distance(b, p) : INF);
}

void ConditionalDistance::boundDistanceToSite(const Point &centre,
                                              DOUBLE halfwidth, const Site *s,
                                              Range &range) const {
  range.erase();
  Range valueA;
  s->boundDistance(a, centre, halfwidth, valueA);
  if (valueA.isEmpty())
    return;
  if (valueA.contains(0))
    range.extendTo(INF);
  if (valueA.is(0, 0))
    return;
  Range valueB;
  s->boundDistance(b, centre, halfwidth, valueB);
  range.extendTo(valueB.lowerBound);
  range.extendTo(valueB.upperBound);
}

UnaryOperatorDistance::UnaryOperatorDistance(const Distance *_a,
                                             const char *_symbol)
    : a(_a), symbol(_symbol) {
  options.useCompositeDistance = true;
}

const std::string UnaryOperatorDistance::description() const {
  std::stringstream s;
  s << symbol << '(' << a->description() << ')';
  return s.str();
}

SiteTypes UnaryOperatorDistance::supportedSites() const {
  return a->supportedSites();
}

LogDistance::LogDistance(const Distance *_a)
    : UnaryOperatorDistance(_a, "ln") {};

DOUBLE LogDistance::distanceToSite(const Point &p, const Site *s) const {
  return log(s->distance(a, p));
}

void LogDistance::boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                      const Site *s, Range &range) const {
  Range valueA;
  s->boundDistance(a, centre, halfwidth, valueA);
  range.erase();
  if (valueA.upperBound < 0)
    return;
  range.lowerBound = valueA.lowerBound <= 0 ? -INF : log(valueA.lowerBound);
  range.upperBound = log(valueA.upperBound);
}

AbsDistance::AbsDistance(const Distance *_a)
    : UnaryOperatorDistance(_a, "abs") {};

DOUBLE AbsDistance::distanceToSite(const Point &p, const Site *s) const {
  return abs(s->distance(a, p));
}

void AbsDistance::boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                      const Site *s, Range &range) const {
  Range valueA;
  s->boundDistance(a, centre, halfwidth, valueA);
  if (valueA.contains(0))
    range.lowerBound = 0;
  else
    range.lowerBound = std::max(-valueA.upperBound, valueA.lowerBound);
  range.upperBound = std::max(-valueA.lowerBound, valueA.upperBound);
}

SquareDistance::SquareDistance(const Distance *_a)
    : UnaryOperatorDistance(_a, "sq") {};

DOUBLE SquareDistance::distanceToSite(const Point &p, const Site *s) const {
  return sqr(s->distance(a, p));
}

void SquareDistance::boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                         const Site *s, Range &range) const {
  Range valueA;
  s->boundDistance(a, centre, halfwidth, valueA);
  DOUBLE sqalw = sqr(valueA.lowerBound);
  DOUBLE sqaup = sqr(valueA.upperBound);
  if (valueA.contains(0))
    range.lowerBound = 0;
  else
    range.lowerBound = std::min(sqalw, sqaup);
  range.upperBound = std::max(sqalw, sqaup);
}

SquareRootDistance::SquareRootDistance(const Distance *_a)
    : UnaryOperatorDistance(_a, "sqrt") {};

DOUBLE SquareRootDistance::distanceToSite(const Point &p, const Site *s) const {
  DOUBLE da = s->distance(a, p);
  if (da < 0)
    Complaint(userError) << "negative value " << da
                         << " for operand to square root operator detected";
  return sqrt(da);
}

void SquareRootDistance::boundDistanceToSite(const Point &centre,
                                             DOUBLE halfwidth, const Site *s,
                                             Range &range) const {
  Range valueA;
  s->boundDistance(a, centre, halfwidth, valueA);

  // the operand is not allowed to be negative:
  if (valueA.upperBound < 0)
    Complaint(userError)
        << "negative value " << valueA.upperBound
        << " for upper bound on operand to square-root operator detected";
  if (valueA.lowerBound < 0)
    valueA.lowerBound = 0;

  range.lowerBound = sqrt(valueA.lowerBound);
  range.upperBound = sqrt(valueA.upperBound);
}

ArccosDistance::ArccosDistance(const Distance *_a)
    : UnaryOperatorDistance(_a, "arccos") {};

DOUBLE ArccosDistance::distanceToSite(const Point &p, const Site *s) const {
  return safeAcos(s->distance(a, p));
}

void ArccosDistance::boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                         const Site *s, Range &range) const {
  range.erase();
  Range valueA;
  s->boundDistance(a, centre, halfwidth, valueA);
  if (!(valueA.upperBound >= -1))
    return;
  if (!(valueA.lowerBound <= 1))
    return;
  range.lowerBound = (valueA.contains(1) ? 0 : acos(valueA.upperBound));
  range.upperBound = (valueA.contains(-1) ? PI : acos(valueA.lowerBound));
}

ArcoshDistance::ArcoshDistance(const Distance *_a)
    : UnaryOperatorDistance(_a, "arcosh") {};

DOUBLE ArcoshDistance::distanceToSite(const Point &p, const Site *s) const {
  return acosh(s->distance(a, p));
}

void ArcoshDistance::boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                         const Site *s, Range &range) const {
  range.erase();
  Range valueA;
  s->boundDistance(a, centre, halfwidth, valueA);
  if (!(valueA.upperBound >= 1))
    return;
  if (valueA.contains(1))
    range.lowerBound = 0;
  else
    range.lowerBound = acosh(valueA.lowerBound);
  range.upperBound = acosh(valueA.upperBound);
}

SiteTypes PointDistance::supportedSites() const { return POINTTYPE; }

SiteTypes SegmentDistance::supportedSites() const { return SEGMENTTYPE; }

SiteTypes PolylineDistance::supportedSites() const { return POLYLINETYPE; }

SiteTypes SetDistance::supportedSites() const {
  return POINTTYPE | SEGMENTTYPE;
}

SiteTypes DirectedPointDistance::supportedSites() const {
  return DIRECTEDPOINTTYPE;
}

SiteTypes BigSiteDistance::supportedSites() const { return BIGTYPE; }

SiteTypes RootedVectorDistance::supportedSites() const {
  return ROOTEDVECTORTYPE;
}

TranslatedDistance::TranslatedDistance(const Distance *_a)
    : UnaryOperatorDistance(_a, "translated") {
  if (!a->supports(ALLTYPES))
    Complaint(userError) << a->description()
                         << " is not defined for trivial sites;\
      therefore it cannot be used in a translated distance measure";
}

SiteTypes TranslatedDistance::supportedSites() const { return POINTTYPE; }

DOUBLE TranslatedDistance::distanceToPointSite(const Point &p,
                                               const PointSite *s) const {
  return a->distanceToSite(p - s->location, s);
}

void TranslatedDistance::boundDistanceToPointSite(const Point &centre,
                                                  DOUBLE halfwidth,
                                                  const PointSite *s,
                                                  Range &range) const {
  a->boundDistanceToSite(centre - s->location, halfwidth, s, range);
}

OrientedDistance::OrientedDistance(const Distance *_a)
    : UnaryOperatorDistance(_a, "oriented") {
  if (!a->supports(BIGTYPE))
    Complaint(userError) << a->description()
                         << " is not defined for trivial sites \
      (possibly with magnitude);\
      therefore it cannot be used in an oriented distance measure";
}

SiteTypes OrientedDistance::supportedSites() const {
  if (a->supports(ALLTYPES))
    return DIRECTEDPOINTTYPE;
  else if (a->supports(BIGTYPE))
    return ROOTEDVECTORTYPE;
  Complaint(internalError) << a->description()
                           << " is not defined for trivial sites \
    (possibly with magnitude); therefore it cannot be used in an oriented distance measure. \
    This error should have been caught when the OrientedDistance object was constructed!";
  return 0;
}

DOUBLE OrientedDistance::distanceToDirectedPointSite(
    const Point &p, const DirectedPointSite *s) const {
  // rotate coordinate system such that site points up,
  // translate such that site is in the centre:
  Point v = p - s->location;
  Point w(-(s->normal * v), s->direction * v);
  return a->distanceToSite(w, s);
}

void OrientedDistance::boundDistanceToDirectedPointSite(
    const Point &centre, DOUBLE halfwidth, const DirectedPointSite *s,
    Range &range) const {
  Point v = centre - s->location;
  Point w(-(s->normal * v), s->direction * v);

  // ratio between diameter of bbox of rotated square and original square:
  Point c(s->normal.x + s->normal.y, s->direction.x + s->direction.y);
  DOUBLE bboxExpansion = std::max(abs(c.x), abs(c.y));

  a->boundDistanceToSite(w, halfwidth * bboxExpansion, s, range);
}

DOUBLE
OrientedDistance::distanceToRootedVectorSite(const Point &p,
                                             const RootedVectorSite *s) const {
  // rotate coordinate system such that site points up,
  // translate such that site is in the centre:
  Point v = p - s->location;
  Point w(-(s->normal * v), s->direction * v);
  return a->distanceToBigSite(w, s);
}

void OrientedDistance::boundDistanceToRootedVectorSite(
    const Point &centre, DOUBLE halfwidth, const RootedVectorSite *s,
    Range &range) const {
  Point v = centre - s->location;
  Point w(-(s->normal * v), s->direction * v);

  // ratio between diameter of bbox of rotated square and original square:
  Point c(s->normal.x + s->normal.y, s->direction.x + s->direction.y);
  DOUBLE bboxExpansion = std::max(abs(c.x), abs(c.y));

  a->boundDistanceToBigSite(w, halfwidth * bboxExpansion, s, range);
}

const std::string EuclideanDistance::description() const { return "Euclidean"; }

DOUBLE EuclideanDistance::distanceToPointSite(const Point &p,
                                              const PointSite *s) const {
  return (p - s->location).length();
}

void EuclideanDistance::boundDistanceToPointSite(const Point &centre,
                                                 DOUBLE halfwidth,
                                                 const PointSite *s,
                                                 Range &range) const {
  DOUBLE radius = halfwidth * sqrt(2.0);
  Point v = centre - s->location;
  range.lowerBound = std::max(v.length() - radius, (DOUBLE)0);
  range.upperBound = v.length() + radius;
}

DOUBLE EuclideanDistance::distanceToSegmentSite(const Point &p,
                                                const SegmentSite *s) const {
  // calculate x-coordinate if axis system is rotated and translated
  // such that the segment points from origin to the right,
  Point v = p - s->origin;
  DOUBLE x = s->direction * v;

  // check if point lies vertically above or below the segment
  if (x > 0 && x < s->length)
    // calculate distance to x-axis under same transformation,
    return abs(s->normal * v);

  // otherwise treat point as out of reach:
  return INF;
}

void EuclideanDistance::boundDistanceToSegmentSite(const Point &centre,
                                                   DOUBLE halfwidth,
                                                   const SegmentSite *s,
                                                   Range &range) const {
  range.erase();
  DOUBLE radius = halfwidth * sqrt(2.0);

  // calculate x-coordinate if axis system is rotated and translated
  // such that the segment points from origin to the right:
  Point v = centre - s->origin;
  DOUBLE x = s->direction * v;

  // calculate distance to the supporting line of the segment
  DOUBLE distance = abs(s->normal * v);

  if (x < -radius || x > s->length + radius)
    return;
  range.lowerBound = std::max(distance - radius, (DOUBLE)0);

  if (x < radius || x > s->length - radius)
    range.upperBound = INF;
  else
    range.upperBound = distance + radius;
}

const std::string SquaredEuclideanDistance::description() const {
  return "squared Euclidean";
}

DOUBLE SquaredEuclideanDistance::distanceToPointSite(const Point &p,
                                                     const PointSite *s) const {
  return (p - s->location).sqr();
}

void SquaredEuclideanDistance::boundDistanceToPointSite(const Point &centre,
                                                        DOUBLE halfwidth,
                                                        const PointSite *s,
                                                        Range &range) const {
  EuclideanDistance().boundDistanceToPointSite(centre, halfwidth, s, range);
  range.lowerBound = sqr(range.lowerBound);
  range.upperBound = sqr(range.upperBound);
}

DOUBLE
SquaredEuclideanDistance::distanceToSegmentSite(const Point &p,
                                                const SegmentSite *s) const {
  // calculate x-coordinate if axis system is rotated and translated
  // such that the segment points from origin to the right,
  Point v = p - s->origin;
  DOUBLE x = s->direction * v;

  // check if point lies vertically above or below the segment
  if (x > 0 && x < s->length)
    // calculate distance to x-axis under same transformation,
    return sqr(s->normal * v);

  // otherwise treat point as out of reach:
  return INF;
}

void SquaredEuclideanDistance::boundDistanceToSegmentSite(const Point &centre,
                                                          DOUBLE halfwidth,
                                                          const SegmentSite *s,
                                                          Range &range) const {
  EuclideanDistance().boundDistanceToSegmentSite(centre, halfwidth, s, range);
  range.lowerBound = sqr(range.lowerBound);
  range.upperBound = sqr(range.upperBound);
}

EuclideanHighwayDistance::EuclideanHighwayDistance() { mapNeeded = true; }

const std::string EuclideanHighwayDistance::description() const {
  return "highway";
}

DOUBLE EuclideanHighwayDistance::distanceToPointSite(const Point &p,
                                                     const PointSite *s) const {
  return map->distance(p, s->siteId);
}

void EuclideanHighwayDistance::boundDistanceToPointSite(const Point &centre,
                                                        DOUBLE halfwidth,
                                                        const PointSite *s,
                                                        Range &range) const {
  DOUBLE distance = distanceToPointSite(centre, s);
  DOUBLE radius = halfwidth * sqrt(2.0);
  range.lowerBound = std::max(distance - radius, (DOUBLE)0);
  range.upperBound = distance + radius;
}

bool EuclideanHighwayDistance::mapNeeded = false;
HighwayMap<EuclideanHighwayEnvironment> *EuclideanHighwayDistance::map = 0;

const std::string ManhattanDistance::description() const { return "L1"; }

DOUBLE ManhattanDistance::distanceToPointSite(const Point &p,
                                              const PointSite *s) const {
  Point v = p - s->location;
  return abs(v.x) + abs(v.y);
}

void ManhattanDistance::boundDistanceToPointSite(const Point &centre,
                                                 DOUBLE halfwidth,
                                                 const PointSite *s,
                                                 Range &range) const {
  DOUBLE radius = halfwidth * 2;
  Point v = centre - s->location;
  range.lowerBound = std::max(abs(v.x) - halfwidth, (DOUBLE)0) +
                     std::max(abs(v.y) - halfwidth, (DOUBLE)0);
  range.upperBound = abs(v.x) + abs(v.y) + radius;
}

DOUBLE ManhattanDistance::distanceToSegmentSite(const Point &p,
                                                const SegmentSite *s) const {
  Point v = p - s->origin;
  if (abs(s->slope) == 1)
    return INF;

  else if (abs(s->slope) < 1) // horizontal tendency
  {
    // test if p.x lies between s.origin.x and s.destination.x,
    // works regardless of which endpoint has the smaller x-coordinate
    if ((p.x >= s->origin.x) == (p.x <= s->destination.x))
      return abs(v.y - v.x * s->slope);
    // out of slab:
    return INF;
  }

  // otherwise: vertical tendency
  if ((p.y >= s->origin.y) == (p.y <= s->destination.y))
    return abs(v.x - v.y * s->invSlope);
  return INF;
}

void ManhattanDistance::boundDistanceToSegmentSite(const Point &centre,
                                                   DOUBLE halfwidth,
                                                   const SegmentSite *s,
                                                   Range &range) const {
  DOUBLE radius = halfwidth * 2;
  range.erase();

  Point v = centre - s->origin;
  if (abs(s->slope) < 1) // horizontal tendency
  {
    if (centre.x < s->bboxMin.x - halfwidth ||
        centre.x > s->bboxMax.x + halfwidth)
      return;
    range.lowerBound = std::max(abs(v.y - v.x * s->slope) - radius, (DOUBLE)0);

    if (centre.x < s->bboxMin.x + halfwidth ||
        centre.x > s->bboxMax.x - halfwidth)
      range.upperBound = INF;
    else
      range.upperBound = abs(v.y - v.x * s->slope) + radius;
    return;
  }

  // otherwise: vertical tendency
  if (centre.y < s->bboxMin.y - halfwidth ||
      centre.y > s->bboxMax.y + halfwidth)
    return;
  range.lowerBound = std::max(abs(v.x - v.y * s->invSlope) - radius, (DOUBLE)0);

  if (centre.y < s->bboxMin.y + halfwidth ||
      centre.y > s->bboxMax.y - halfwidth)
    range.upperBound = INF;
  else
    range.upperBound = abs(v.x - v.y * s->invSlope) + radius;
}

ManhattanHighwayDistance::ManhattanHighwayDistance() { mapNeeded = true; }

const std::string ManhattanHighwayDistance::description() const {
  return "manhattan highway";
}

DOUBLE ManhattanHighwayDistance::distanceToPointSite(const Point &p,
                                                     const PointSite *s) const {
  return map->distance(p, s->siteId);
}

void ManhattanHighwayDistance::boundDistanceToPointSite(const Point &centre,
                                                        DOUBLE halfwidth,
                                                        const PointSite *s,
                                                        Range &range) const {
  DOUBLE distance = distanceToPointSite(centre, s);
  DOUBLE radius = halfwidth * 2.0;
  range.lowerBound = std::max(distance - radius, (DOUBLE)0);
  range.upperBound = distance + radius;
}

bool ManhattanHighwayDistance::mapNeeded = false;
HighwayMap<ManhattanHighwayEnvironment> *ManhattanHighwayDistance::map = 0;

const std::string cTriangularGridDistance::description() const {
  return "@triangular";
}

DOUBLE cTriangularGridDistance::distanceToSite(const Point &p,
                                               const Site *s) const {
  static const DOUBLE ROOTTHIRD = 1.0 / sqrt(3);
  DOUBLE pa = p.y * ROOTTHIRD;
  DOUBLE pb = 0.5 * (-p.x - pa);
  DOUBLE pc = -pb - pa;
  return abs(pa) + abs(pb) + abs(pc);
}

void cTriangularGridDistance::boundDistanceToSite(const Point &centre,
                                                  DOUBLE halfwidth,
                                                  const Site *s,
                                                  Range &range) const {
  static const DOUBLE unitSquareDiameter = sqrt(3.0) / 3 + 1;
  DOUBLE distance = distanceToSite(centre, s);
  range.lowerBound =
      std::max(distance - halfwidth * unitSquareDiameter, (DOUBLE)0);
  range.upperBound = distance + halfwidth * unitSquareDiameter;
}

template <class P>
SphericalDistance<P>::SphericalDistance(const DOUBLE _aspectRatio,
                                        const DOUBLE _radius)
    : aspectRatio(_aspectRatio), radius(_radius), cosRadius(cos(radius * PI)){};

template <class P> const std::string SphericalDistance<P>::description() const {
  std::stringstream s;
  s << "spherical(" << P::description() << ',' << aspectRatio << ',' << radius
    << ')';
  return s.str();
}

template <class P> SiteTypes SphericalDistance<P>::supportedSites() const {
  return PointDistance::supportedSites() | SPHERICALPOINTTYPE;
}

template <class P>
DOUBLE SphericalDistance<P>::distanceToPointSite(const Point &p,
                                                 const PointSite *s) const {
  SphericalPoint pos;
  if (!P::projectOntoSphere(Point(p.x, p.y * aspectRatio), pos))
    return NAN;
  if (pos * P::centre < cosRadius)
    return NAN;
  SphericalPoint sos;
  if (!P::projectOntoSphere(Point(s->location.x, s->location.y * aspectRatio),
                            sos))
    return NAN;
  return safeAcos(pos * sos);
}

template <class P>
void SphericalDistance<P>::boundDistanceToPointSite(const Point &centre,
                                                    DOUBLE halfwidth,
                                                    const PointSite *s,
                                                    Range &range) const {
  range.erase();

  // This function is likely to be called with the same centre and
  // halfwidth in succession (once for each site). Save the result
  // of the expensive projection:
  static Point lastCentre;
  static DOUBLE lastHalfwidth = -1; // negative for no previous query
  static SphericalPoint pos;
  static DOUBLE radius; // negative for invalid
  if (!(centre == lastCentre && halfwidth == lastHalfwidth)) {
    P::projectOntoSphere(Point(centre.x, centre.y * aspectRatio),
                         Point(halfwidth, halfwidth * aspectRatio), pos,
                         radius);
    lastCentre = centre;
    lastHalfwidth = halfwidth;
  }
  if (radius < 0)
    return;
  if (safeAcos(pos * P::centre) - radius > safeAcos(cosRadius))
    return;
  SphericalPoint sos;
  if (!P::projectOntoSphere(Point(s->location.x, s->location.y * aspectRatio),
                            sos))
    return;
  DOUBLE centreDistance = safeAcos(pos * sos);
  range.set(centreDistance - radius, centreDistance + radius);
}

template <class P>
DOUBLE SphericalDistance<P>::distanceToSphericalPointSite(
    const Point &p, const SphericalPointSite *s) const {
  SphericalPoint pos;
  if (!P::projectOntoSphere(Point(p.x, p.y * aspectRatio), pos))
    return NAN;
  if (pos * P::centre < cosRadius)
    return NAN;
  return safeAcos(pos * s->sphericalLocation);
}

template <class P>
void SphericalDistance<P>::boundDistanceToSphericalPointSite(
    const Point &centre, DOUBLE halfwidth, const SphericalPointSite *s,
    Range &range) const {
  range.erase();

  // This function is likely to be called with the same centre and
  // halfwidth in succession (once for each site). Save the result
  // of the expensive projection:
  static Point lastCentre;
  static DOUBLE lastHalfwidth = -1; // negative for no previous query
  static SphericalPoint pos;
  static DOUBLE radius; // negative for invalid
  if (!(centre == lastCentre && halfwidth == lastHalfwidth)) {
    P::projectOntoSphere(Point(centre.x, centre.y * aspectRatio),
                         Point(halfwidth, halfwidth * aspectRatio), pos,
                         radius);
    lastCentre = centre;
    lastHalfwidth = halfwidth;
  }
  if (radius < 0)
    return;
  if (safeAcos(pos * P::centre) - radius > safeAcos(cosRadius))
    return;
  DOUBLE centreDistance = safeAcos(pos * s->sphericalLocation);
  range.set(centreDistance - radius, centreDistance + radius);
}

template <class Q> const std::string AzimuthalProjection<Q>::description() {
  return Q::description;
}

template <class Q>
const SphericalPoint AzimuthalProjection<Q>::centre = SphericalPoint(0, 0, 1);

template <class Q>
bool AzimuthalProjection<Q>::projectOntoSphere(const Point &inPlane,
                                               SphericalPoint &onSphere) {
  DOUBLE ps = inPlane * inPlane;
  DOUBLE height;
  DOUBLE scale;
  Q::getHeightAndScale(ps, height, scale);
  if (!(height >= -1 && height <= 1))
    return false;
  onSphere = SphericalPoint(scale * inPlane.x, scale * inPlane.y, height);
  return true;
}

template <class Q>
void AzimuthalProjection<Q>::projectOntoSphere(const Point &inPlane,
                                               const Point &halfWidth,
                                               SphericalPoint &centre,
                                               DOUBLE &radius) {
  radius = -1; // default answer
  Range height;
  Range longitude;

  Point corner[4] = {Point(inPlane.x - halfWidth.x, inPlane.y - halfWidth.y),
                     Point(inPlane.x - halfWidth.x, inPlane.y + halfWidth.y),
                     Point(inPlane.x + halfWidth.x, inPlane.y + halfWidth.y),
                     Point(inPlane.x + halfWidth.x, inPlane.y - halfWidth.y)};

  // determine minimum square radius:
  DOUBLE sqr2 = INF;
  if (nonDecreasing(corner[1].x, 0, corner[2].x)) {
    if (nonDecreasing(corner[0].y, 0, corner[1].y))
      sqr2 = 0;
    else
      sqr2 = sqr(abs(inPlane.y) - halfWidth.y);
  } else if (nonDecreasing(corner[0].y, 0, corner[1].y))
    sqr2 = sqr(abs(inPlane.x) - halfWidth.x);
  else
    for (int i = 0; i < 4; ++i)
      keepMin(sqr2, corner[i].sqr());

  // determine maximum square radius:
  DOUBLE sqr1 = 0;
  for (int i = 0; i < 4; ++i)
    keepMax(sqr1, corner[i].sqr());

  // determine minimum latitude:
  height.lowerBound = safeMax(Q::height(sqr1), (DOUBLE)-1);

  if (sqr2 == 0) {
    // patch contains pole
    centre = SphericalPoint(0, 0, 1);
    radius = PI;
    return;
  }

  // determine maximum latitude:
  height.upperBound = Q::height(sqr2);

  if (!(height.upperBound >= -1))
    // patch lies entirely outside projection
    return;

  bool bot = (corner[1].y <= 0);
  bool top = (corner[0].y >= 0);
  bool lft = (corner[2].x <= 0);
  bool rgt = (corner[1].x >= 0);

  if (rgt && !top)
    longitude.lowerBound = atan2(corner[0].y, corner[0].x);
  else if (top && !lft)
    longitude.lowerBound = atan2(corner[3].y, corner[3].x);
  else if (lft && !bot)
    longitude.lowerBound = atan2(corner[2].y, corner[2].x);
  else
    // (bot && !rgt)
    longitude.lowerBound = atan2(corner[1].y, corner[1].x);

  if (rgt && !bot)
    longitude.upperBound = atan2(corner[1].y, corner[1].x);
  else if (bot && !lft)
    longitude.upperBound = atan2(corner[2].y, corner[2].x);
  else if (lft && !top)
    longitude.upperBound = atan2(corner[3].y, corner[3].x);
  else
    // (top && !rgt)
    longitude.upperBound = atan2(corner[0].y, corner[0].x);
  if (longitude.upperBound < longitude.lowerBound)
    longitude.upperBound += TWOPI;

  getCentreOfSphericalRectangle(height, longitude, centre, radius);
}

const std::string AzimuthalEqualAreaProjection::description =
    "azimuthal equal-area";
DOUBLE AzimuthalEqualAreaProjection::height(const DOUBLE rr) {
  // with south pole on unit circle:
  return (DOUBLE)1 - rr * 2;
}
void AzimuthalEqualAreaProjection::getHeightAndScale(const DOUBLE rr,
                                                     DOUBLE &_height,
                                                     DOUBLE &_scale) {
  _height = height(rr);
  _scale = sqrt((DOUBLE)1 - rr) * 2;
}

const std::string StereographicProjection::description = "stereographic";
DOUBLE StereographicProjection::height(const DOUBLE rr) {
  // with equator on unit circle:
  return (-rr + 1) / (rr + 1);
}
void StereographicProjection::getHeightAndScale(const DOUBLE rr,
                                                DOUBLE &_height,
                                                DOUBLE &_scale) {
  _height = height(rr);
  _scale = (DOUBLE)2 / (rr + 1);
}

const std::string EquidistantProjection::description = "azimuthal equidistant";
DOUBLE EquidistantProjection::height(const DOUBLE rr) {
  // with south pole on unit circle:
  return (rr <= 1 ? cos(PI * sqrt(rr)) : NAN);
}
void EquidistantProjection::getHeightAndScale(const DOUBLE rr, DOUBLE &_height,
                                              DOUBLE &_scale) {
  if (rr > 1) {
    _height = NAN;
    return;
  }
  DOUBLE r = sqrt(rr);
  DOUBLE latitude = HALFPI - PI * r;
  _height = sin(latitude);
  _scale = (r == 0 ? 1 : cos(latitude) / r);
}

const std::string GnomonicProjection::description = "gnomonic";
DOUBLE GnomonicProjection::height(const DOUBLE rr) {
  return sqrt((DOUBLE)1 / (rr + 1));
}
void GnomonicProjection::getHeightAndScale(const DOUBLE rr, DOUBLE &_height,
                                           DOUBLE &_scale) {
  _height = height(rr);
  _scale = _height;
}

const std::string OrthographicProjection::description = "orthographic";
DOUBLE OrthographicProjection::height(const DOUBLE rr) {
  return (rr <= 1 ? sqrt(-rr + 1) : NAN);
}
void OrthographicProjection::getHeightAndScale(const DOUBLE rr, DOUBLE &_height,
                                               DOUBLE &_scale) {
  _height = height(rr);
  _scale = 1;
}

template <class Q> const std::string CylindricalProjection<Q>::description() {
  return Q::description;
}

template <class Q>
const SphericalPoint CylindricalProjection<Q>::centre = SphericalPoint(1, 0, 0);

template <class Q>
bool CylindricalProjection<Q>::projectOntoSphere(const Point &inPlane,
                                                 SphericalPoint &onSphere) {
  DOUBLE px = inPlane.x;
  if (px < -1 || px > 1)
    return false;

  onSphere.z = Q::height(inPlane.y);
  if (!(onSphere.z >= -1 && onSphere.z <= 1))
    return false;

  DOUBLE pr = sqrt((DOUBLE)1 - sqr(onSphere.z));
  DOUBLE pt = PI * px;
  onSphere.x = pr * cos(pt);
  onSphere.y = pr * sin(pt);

  return true;
}

template <class Q>
void CylindricalProjection<Q>::projectOntoSphere(const Point &inPlane,
                                                 const Point &halfWidth,
                                                 SphericalPoint &centre,
                                                 DOUBLE &radius) {
  radius = -1; // default answer
  Range height;
  Range longitude;
  longitude.lowerBound = std::max(inPlane.x - halfWidth.x, (DOUBLE)-1.0);
  longitude.upperBound = std::min(inPlane.x + halfWidth.x, (DOUBLE)1.0);
  if (longitude.lowerBound > 1 || longitude.upperBound < -1)
    return;
  longitude.lowerBound *= PI;
  longitude.upperBound *= PI;
  height.lowerBound = Q::height(inPlane.y - halfWidth.y);
  height.upperBound = Q::height(inPlane.y + halfWidth.y);
  if (inPlane.y - halfWidth.y > 0 && isnan(height.lowerBound))
    return;
  if (inPlane.y + halfWidth.y < 0 && isnan(height.upperBound))
    return;
  keepMax(height.lowerBound, (DOUBLE)-1);
  keepMin(height.upperBound, (DOUBLE)1);
  getCentreOfSphericalRectangle(height, longitude, centre, radius);
}

const std::string CentralCylindricalProjection::description =
    "central cylindrical";
DOUBLE CentralCylindricalProjection::height(const DOUBLE y) {
  return y / sqrt(sqr(y) + 1 / sqr(PI));
}

const std::string CylindricalEqualAreaProjection::description =
    "cylindrical equal-area";
DOUBLE CylindricalEqualAreaProjection::height(const DOUBLE y) {
  DOUBLE h = PI * y;
  return (h >= -1 && h <= 1 ? h : NAN);
}

const std::string EquirectangularProjection::description = "equirectangular";
DOUBLE EquirectangularProjection::height(const DOUBLE y) {
  return (y >= -0.5 && y <= 0.5 ? sin(PI * y) : NAN);
}

const std::string MercatorProjection::description = "Mercator";
DOUBLE MercatorProjection::height(const DOUBLE y) {
  return -cos(atan(exp(PI * y)) * 2);
}

const std::string MollweideProjection::description() {
  return std::string("Mollweide");
}

const SphericalPoint MollweideProjection::centre = SphericalPoint(1, 0, 0);

bool MollweideProjection::projectOntoSphere(const Point &inPlane,
                                            SphericalPoint &onSphere) {
  if (inPlane.sqr() > 1)
    return false;

  // from wikipedia, adapted to squash into a unit disk:
  // theta = arcsin(y)
  // latitude = arcsin(2 theta + sin 2 theta)/pi = arcsin(2 theta + 2 y cos
  // theta) longitude = pi x / cos theta

  DOUBLE theta = safeAsin(inPlane.y);
  DOUBLE costheta = cos(theta);
  onSphere = SphericalPoint(safeAsin((theta + inPlane.y * costheta) * 2.0 / PI),
                            PI * inPlane.x / costheta);
  return true;
}

void MollweideProjection::projectOntoSphere(const Point &inPlane,
                                            const Point &halfWidth,
                                            SphericalPoint &centre,
                                            DOUBLE &radius) {
  radius = -1; // default answer
  Range height;
  Range longitude;

  DOUBLE y1 = std::max(inPlane.y - halfWidth.y, (DOUBLE)-1.0);
  DOUBLE y2 = std::min(inPlane.y + halfWidth.y, (DOUBLE)1.0);
  if (y1 > 1 || y2 < -1)
    return;

  DOUBLE costheta1 = sqrt((DOUBLE)1.0 - sqr(y1));
  DOUBLE costheta2 = sqrt((DOUBLE)1.0 - sqr(y2));
  DOUBLE mincostheta = std::min(costheta1, costheta2);
  DOUBLE maxcostheta =
      (y1 <= 0 && y2 >= 0 ? 1 : std::max(costheta1, costheta2));

  longitude.lowerBound = inPlane.x - halfWidth.x;
  if (longitude.lowerBound < 0)
    longitude.lowerBound /= mincostheta;
  else
    longitude.lowerBound /= maxcostheta;
  keepMax(longitude.lowerBound, -1.0);
  longitude.upperBound = inPlane.x + halfWidth.x;
  if (longitude.upperBound < 0)
    longitude.upperBound /= maxcostheta;
  else
    longitude.upperBound /= mincostheta;
  keepMin(longitude.upperBound, 1.0);
  if (longitude.lowerBound > 1 || longitude.upperBound < -1)
    return;
  longitude.lowerBound *= PI;
  longitude.upperBound *= PI;

  DOUBLE theta1 = safeAsin(y1);
  height.lowerBound = (theta1 * 2.0 + y1 * costheta1 * 2.0) / PI;
  DOUBLE theta2 = safeAsin(y2);
  height.upperBound = (theta2 * 2.0 + y2 * costheta2 * 2.0) / PI;

  getCentreOfSphericalRectangle(height, longitude, centre, radius);
}

const std::string SinusoidalProjection::description() {
  return std::string("sinusoidal");
}

const SphericalPoint SinusoidalProjection::centre = SphericalPoint(1, 0, 0);

bool SinusoidalProjection::projectOntoSphere(const Point &inPlane,
                                             SphericalPoint &onSphere) {
  if (sqr(inPlane.y) > 1)
    return false;
  DOUBLE theta = inPlane.y * HALFPI;
  DOUBLE fi = inPlane.x / cos(theta);
  if (sqr(fi) > 1)
    return false;
  onSphere = SphericalPoint(theta, PI * fi);
  return true;
}

void SinusoidalProjection::projectOntoSphere(const Point &inPlane,
                                             const Point &halfWidth,
                                             SphericalPoint &centre,
                                             DOUBLE &radius) {
  radius = -1; // default answer
  Range height;
  Range longitude;

  DOUBLE theta1 = std::max(inPlane.y - halfWidth.y, (DOUBLE)-1.0);
  DOUBLE theta2 = std::min(inPlane.y + halfWidth.y, (DOUBLE)1.0);
  if (theta1 > 1 || theta2 < -1)
    return;
  theta1 *= HALFPI;
  theta2 *= HALFPI;

  DOUBLE costheta1 = cos(theta1);
  DOUBLE costheta2 = cos(theta2);
  DOUBLE mincostheta = std::min(costheta1, costheta2);
  DOUBLE maxcostheta =
      (theta1 <= 0 && theta2 >= 0 ? 1 : std::max(costheta1, costheta2));

  longitude.lowerBound = inPlane.x - halfWidth.x;
  if (longitude.lowerBound < 0)
    longitude.lowerBound /= mincostheta;
  else
    longitude.lowerBound /= maxcostheta;
  keepMax(longitude.lowerBound, -1.0);
  longitude.upperBound = inPlane.x + halfWidth.x;
  if (longitude.upperBound < 0)
    longitude.upperBound /= maxcostheta;
  else
    longitude.upperBound /= mincostheta;
  keepMin(longitude.upperBound, 1.0);
  if (longitude.lowerBound > 1 || longitude.upperBound < -1)
    return;
  longitude.lowerBound *= PI;
  longitude.upperBound *= PI;

  height.lowerBound = sin(theta1);
  height.upperBound = sin(theta2);

  getCentreOfSphericalRectangle(height, longitude, centre, radius);
}

const std::string HammerProjection::description() {
  return std::string("Hammer");
}

const SphericalPoint HammerProjection::centre = SphericalPoint(1, 0, 0);

bool HammerProjection::projectOntoSphere(const Point &inPlane,
                                         SphericalPoint &onSphere) {
  if (inPlane.sqr() > 1)
    return false;
  // shrink to hemisphere:
  Point azimuthal = inPlane * SQRTTWO * 0.5;
  AzimuthalProjection<AzimuthalEqualAreaProjection>::projectOntoSphere(
      azimuthal, onSphere);
  // expand to sphere:
  onSphere.stretchHemisphereToSphere();
  return true;
}

void HammerProjection::projectOntoSphere(const Point &inPlane,
                                         const Point &halfWidth,
                                         SphericalPoint &centre,
                                         DOUBLE &radius) {
  Point azimuthal = inPlane * SQRTTWO * 0.5;
  AzimuthalProjection<AzimuthalEqualAreaProjection>::projectOntoSphere(
      azimuthal, halfWidth, centre, radius);
  centre.stretchHemisphereToSphere();
  radius *= 2;
}

const std::string AitoffProjection::description() {
  return std::string("Aitoff");
}

const SphericalPoint AitoffProjection::centre = SphericalPoint(1, 0, 0);

bool AitoffProjection::projectOntoSphere(const Point &inPlane,
                                         SphericalPoint &onSphere) {
  if (inPlane.sqr() > 1)
    return false;
  // shrink to hemisphere:
  Point azimuthal = inPlane * 0.5;
  AzimuthalProjection<EquidistantProjection>::projectOntoSphere(azimuthal,
                                                                onSphere);
  // expand to sphere:
  onSphere.stretchHemisphereToSphere();
  return true;
}

void AitoffProjection::projectOntoSphere(const Point &inPlane,
                                         const Point &halfWidth,
                                         SphericalPoint &centre,
                                         DOUBLE &radius) {
  Point azimuthal = inPlane * 0.5;
  AzimuthalProjection<EquidistantProjection>::projectOntoSphere(
      azimuthal, halfWidth, centre, radius);
  centre.stretchHemisphereToSphere();
  radius *= 2;
}

template <int N> const std::string PetalProjection<N>::description() {
  return std::string(1, '0' + N) + std::string("-leaf");
}

template <int N>
bool PetalProjection<N>::projectOntoSphere(const Point &inPlane,
                                           SphericalPoint &onSphere) {
  // latitude t is shown on fraction cos t / (pi/2 - t)
  // of circle with radius pi/2 - t and circumference 2 pi (pi/2 - t)

  DOUBLE r = inPlane.length();
  if (r > 1)
    return false;
  if (r == 0) {
    onSphere = SphericalPoint(HALFPI, 0);
    return true;
  }
  DOUBLE lat = HALFPI - r * PI;                  // * R
  DOUBLE halfLeafAngularSize = cos(lat) / r / N; // * R
  // calculate direction in [0, 2pi], bot is 0:
  DOUBLE lng = atan2(inPlane.x, -inPlane.y) + PI;
  for (int i = 0; i < N; ++i) {
    if (lng > (i + 1) * TWOPI / N)
      continue;
    DOUBLE rellng = lng - (0.5 + i) * TWOPI / N;
    if (abs(rellng) > halfLeafAngularSize)
      return false;
    lng = (0.5 + i + rellng * 0.5 / halfLeafAngularSize) * TWOPI / N;
    break;
  }
  // right is 0:
  onSphere = SphericalPoint(lat, lng + HALFPI);
  return true;
}

template <int N>
void PetalProjection<N>::projectOntoSphere(const Point &inPlane,
                                           const Point &halfWidth,
                                           SphericalPoint &centre,
                                           DOUBLE &radius) {
  // just trivial bounds for now:
  centre = SphericalPoint(0, 0, 1);
  radius = PI;
}

const std::string WernerProjection::description() { return "werner"; }

const SphericalPoint WernerProjection::centre = SphericalPoint(0, 0, 1);

bool WernerProjection::projectOntoSphere(const Point &inPlane,
                                         SphericalPoint &onSphere) {
  return PetalProjection<1>::projectOntoSphere(inPlane, onSphere);
}

void WernerProjection::projectOntoSphere(const Point &inPlane,
                                         const Point &halfWidth,
                                         SphericalPoint &centre,
                                         DOUBLE &radius) {
  // just a sloppy conservative check if the window is not outside the map:

  radius = -1;

  // check against bounding box:
  if (inPlane.x + halfWidth.x < -0.65)
    return;
  if (inPlane.x - halfWidth.x > 0.65)
    return;
  if (inPlane.y + halfWidth.y < -1)
    return;
  if (inPlane.y - halfWidth.y > 0.30)
    return;

  // check against unit disk:
  Point canonicalPoint(std::max(abs(inPlane.x) - halfWidth.x, (DOUBLE)0),
                       std::max(abs(inPlane.y) - halfWidth.y, (DOUBLE)0));
  if (canonicalPoint.sqr() >= 1)
    return;

  centre = SphericalPoint(0, 0, 1);
  radius = PI;
}

const std::string cLMinDistance::description() const { return "@Lminf"; }

DOUBLE cLMinDistance::distanceToSite(const Point &p, const Site *s) const {
  return std::min(abs(p.x), abs(p.y));
}

void cLMinDistance::boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                        const Site *s, Range &range) const {
  DOUBLE xLowerBound = 0;
  keepMax(xLowerBound, centre.x - halfwidth);
  keepMax(xLowerBound, -centre.x - halfwidth);
  DOUBLE yLowerBound = 0;
  keepMax(yLowerBound, centre.y - halfwidth);
  keepMax(yLowerBound, -centre.y - halfwidth);
  range.lowerBound = std::min(xLowerBound, yLowerBound);

  DOUBLE xUpperBound = 0;
  keepMax(xUpperBound, centre.x + halfwidth);
  keepMax(xUpperBound, -centre.x + halfwidth);
  DOUBLE yUpperBound = 0;
  keepMax(yUpperBound, centre.y + halfwidth);
  keepMax(yUpperBound, -centre.y + halfwidth);
  range.upperBound = std::min(xUpperBound, yUpperBound);
}

const std::string cVolumeDistance::description() const { return "@L0"; }

DOUBLE cVolumeDistance::distanceToSite(const Point &p, const Site *s) const {
  return abs(p.x * p.y);
}

void cVolumeDistance::boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                          const Site *s, Range &range) const {
  DOUBLE xLowerBound = 0;
  keepMax(xLowerBound, centre.x - halfwidth);
  keepMax(xLowerBound, -centre.x - halfwidth);
  DOUBLE yLowerBound = 0;
  keepMax(yLowerBound, centre.y - halfwidth);
  keepMax(yLowerBound, -centre.y - halfwidth);
  range.lowerBound = xLowerBound * yLowerBound;

  DOUBLE xUpperBound = 0;
  keepMax(xUpperBound, centre.x + halfwidth);
  keepMax(xUpperBound, -centre.x + halfwidth);
  DOUBLE yUpperBound = 0;
  keepMax(yUpperBound, centre.y + halfwidth);
  keepMax(yUpperBound, -centre.y + halfwidth);
  range.upperBound = xUpperBound * yUpperBound;
}

const std::string cLMaxDistance::description() const { return "@Linf"; }

DOUBLE cLMaxDistance::distanceToSite(const Point &p, const Site *s) const {
  return std::max(abs(p.x), abs(p.y));
}

void cLMaxDistance::boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                        const Site *s, Range &range) const {
  DOUBLE xLowerBound = 0;
  keepMax(xLowerBound, centre.x - halfwidth);
  keepMax(xLowerBound, -centre.x - halfwidth);
  DOUBLE yLowerBound = 0;
  keepMax(yLowerBound, centre.y - halfwidth);
  keepMax(yLowerBound, -centre.y - halfwidth);
  range.lowerBound = std::max(xLowerBound, yLowerBound);

  DOUBLE xUpperBound = 0;
  keepMax(xUpperBound, centre.x + halfwidth);
  keepMax(xUpperBound, -centre.x + halfwidth);
  DOUBLE yUpperBound = 0;
  keepMax(yUpperBound, centre.y + halfwidth);
  keepMax(yUpperBound, -centre.y + halfwidth);
  range.upperBound = std::max(xUpperBound, yUpperBound);
}

cLDistance::cLDistance(const DOUBLE _n) : n(_n) {
  if (n == 0)
    Complaint(internalError) << "L0-distance is undefined";
  invn = ((DOUBLE)1.0) / n;
}

const std::string cLDistance::description() const {
  std::stringstream s;
  s << "@L" << n;
  return s.str();
}

DOUBLE cLDistance::distanceToSite(const Point &p, const Site *s) const {
  if (n < 0 && (p.x == 0 || p.y == 0))
    return 0;
  return pow(pow(abs(p.x), n) + pow(abs(p.y), n), invn);
}

void cLDistance::boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                     const Site *s, Range &range) const {
  DOUBLE xLowerBound = 0;
  keepMax(xLowerBound, centre.x - halfwidth);
  keepMax(xLowerBound, -centre.x - halfwidth);
  DOUBLE yLowerBound = 0;
  keepMax(yLowerBound, centre.y - halfwidth);
  keepMax(yLowerBound, -centre.y - halfwidth);
  range.lowerBound = 0;
  if (n > 0 || (xLowerBound != 0 && yLowerBound != 0))
    range.lowerBound = pow(pow(xLowerBound, n) + pow(yLowerBound, n), invn);

  DOUBLE xUpperBound = 0;
  keepMax(xUpperBound, centre.x + halfwidth);
  keepMax(xUpperBound, -centre.x + halfwidth);
  DOUBLE yUpperBound = 0;
  keepMax(yUpperBound, centre.y + halfwidth);
  keepMax(yUpperBound, -centre.y + halfwidth);
  range.upperBound = pow(pow(xUpperBound, n) + pow(yUpperBound, n), invn);
}

template <class P> HyperbolicDistance<P>::HyperbolicDistance(){};

template <class P>
const std::string HyperbolicDistance<P>::description() const {
  std::stringstream s;
  s << "cosh hyperbolic(" << P::description << ')';
  return s.str();
}

template <class P> SiteTypes HyperbolicDistance<P>::supportedSites() const {
  return PointDistance::supportedSites() | HYPERBOLICPOINTTYPE;
}

template <class P>
DOUBLE HyperbolicDistance<P>::distanceToPointSite(const Point &p,
                                                  const PointSite *s) const {
  GansPoint ph;
  if (!P::projectOntoGansPlane(p, ph))
    return NAN;
  GansPoint sh;
  if (!P::projectOntoGansPlane(s->location, sh))
    return NAN;
  return coshdistance(ph, sh);
}

template <class P>
void HyperbolicDistance<P>::boundDistanceToPointSite(const Point &centre,
                                                     DOUBLE halfwidth,
                                                     const PointSite *s,
                                                     Range &range) const {
  range.erase();

  // This function is likely to be called with the same centre and
  // halfwidth in succession (once for each site). Save the result
  // of the expensive projection:
  static Point lastCentre;
  static DOUBLE lastHalfwidth = -1; // negative for no previous query
  static GansPoint pos;
  static DOUBLE radius; // negative for invalid
  if (!(centre == lastCentre && halfwidth == lastHalfwidth)) {
    P::projectOntoGansPlane(Point(centre.x, centre.y),
                            Point(halfwidth, halfwidth), pos, radius);
    lastCentre = centre;
    lastHalfwidth = halfwidth;
  }
  if (radius < 0)
    return;
  GansPoint sos;
  if (!P::projectOntoGansPlane(s->location, sos))
    return;
  DOUBLE centreDistance = coshdistance(pos, sos);
  range.set(centreDistance - radius, centreDistance + radius);
}

template <class P>
DOUBLE HyperbolicDistance<P>::distanceToHyperbolicPointSite(
    const Point &p, const HyperbolicPointSite *s) const {
  GansPoint ph;
  if (!P::projectOntoGansPlane(p, ph))
    return NAN;
  return coshdistance(ph, s->hyperbolicLocation);
}

template <class P>
void HyperbolicDistance<P>::boundDistanceToHyperbolicPointSite(
    const Point &centre, DOUBLE halfwidth, const HyperbolicPointSite *s,
    Range &range) const {
  range.erase();

  // This function is likely to be called with the same centre and
  // halfwidth in succession (once for each site). Save the result
  // of the expensive projection:
  static Point lastCentre;
  static DOUBLE lastHalfwidth = -1; // negative for no previous query
  static GansPoint pos;
  static DOUBLE radius; // negative for invalid
  if (!(centre == lastCentre && halfwidth == lastHalfwidth)) {
    P::projectOntoGansPlane(Point(centre.x, centre.y),
                            Point(halfwidth, halfwidth), pos, radius);
    lastCentre = centre;
    lastHalfwidth = halfwidth;
  }
  if (radius < 0)
    return;
  DOUBLE centreDistance = coshdistance(pos, s->hyperbolicLocation);
  range.set(centreDistance - radius, centreDistance + radius);
}

const std::string GansProjection::description = "Gans";

bool GansProjection::projectOntoGansPlane(const Point &preimage,
                                          GansPoint &image) {
  image = preimage;
  return true;
}

void GansProjection::projectOntoGansPlane(const Point &inPlane,
                                          const Point &halfWidth,
                                          GansPoint &centre, DOUBLE &radius) {
  // just a place-holder:
  centre = GansPoint(0, 0);
  radius = INF;
}

const std::string InvertedGansProjection::description = "inverted";

bool InvertedGansProjection::projectOntoGansPlane(const Point &preimage,
                                                  GansPoint &image) {
  if (preimage.inOrigin())
    return false;
  image = preimage / preimage.sqr();
  return true;
}

void InvertedGansProjection::projectOntoGansPlane(const Point &inPlane,
                                                  const Point &halfWidth,
                                                  GansPoint &centre,
                                                  DOUBLE &radius) {
  // just a place-holder:
  centre = GansPoint(0, 0);
  radius = INF;
}

const std::string PoincareDiskProjection::description = "Poincare disk";

bool PoincareDiskProjection::projectOntoGansPlane(const Point &preimage,
                                                  GansPoint &image) {
  DOUBLE sqd = preimage.sqr();
  if (sqd >= 1)
    return false;
  image = preimage * ((DOUBLE)2 / ((DOUBLE)1.0 - sqd));
  return true;
}

void PoincareDiskProjection::projectOntoGansPlane(const Point &inPlane,
                                                  const Point &halfWidth,
                                                  GansPoint &centre,
                                                  DOUBLE &radius) {
  // only checking whether the window does not lie outside the disk
  radius = -1;

  // calculate distance to origin
  Point canonicalPoint(std::max(abs(inPlane.x) - halfWidth.x, (DOUBLE)0),
                       std::max(abs(inPlane.y) - halfWidth.y, (DOUBLE)0));
  if (canonicalPoint.sqr() >= 1)
    return;

  centre = GansPoint(0, 0);
  radius = INF;
}

const std::string PoincareHalfplaneProjection::description =
    "Poincare halfplane";

bool PoincareHalfplaneProjection::projectOntoGansPlane(const Point &preimage,
                                                       GansPoint &image) {
  // shift the halfplane such that the Euclidean centre of the unit disk
  // lies in the origin
  static const DOUBLE horizon = -1;

  if (preimage.y <= horizon)
    return false;

  //    via Poincare disk:
  //  Point p(preimage.x, horizon - preimage.y - 1);
  //  DOUBLE sqd = (DOUBLE) 2 / p.sqr();
  //  return PoincareDiskProjection::projectOntoGansPlane(p * sqd + Point(0,1),
  //  image);
  //    or more directly:
  Point p(preimage.x, preimage.y - horizon);
  image.x = p.x / p.y;
  image.y = 0.5 * (p.sqr() - 1) / p.y;
  return true;
};

void PoincareHalfplaneProjection::projectOntoGansPlane(const Point &inPlane,
                                                       const Point &halfWidth,
                                                       GansPoint &centre,
                                                       DOUBLE &radius) {
  // only checking whether the window does not lie outside the halfplane

  // shift the halfplane such that the Euclidean centre of the unit disk
  // lies in the origin
  static const DOUBLE horizon = -1;

  radius = -1;
  if (inPlane.y + halfWidth.y <= horizon)
    return;
  centre = GansPoint(0, 0);
  radius = INF;
}

const std::string KleinProjection::description = "Klein";

bool KleinProjection::projectOntoGansPlane(const Point &preimage,
                                           GansPoint &image) {
  DOUBLE sqd = preimage.sqr();
  if (sqd >= 1)
    return false;
  image = preimage * ((DOUBLE)1 / sqrt((DOUBLE)1.0 - sqd));
  return true;
}

void KleinProjection::projectOntoGansPlane(const Point &inPlane,
                                           const Point &halfWidth,
                                           GansPoint &centre, DOUBLE &radius) {
  // only checking whether the window does not lie outside the disk

  radius = -1;

  // calculate distance to origin
  Point canonicalPoint(std::max(abs(inPlane.x) - halfWidth.x, (DOUBLE)0),
                       std::max(abs(inPlane.y) - halfWidth.y, (DOUBLE)0));
  if (canonicalPoint.sqr() >= 1)
    return;

  centre = GansPoint(0, 0);
  radius = INF;
}

const std::string HyperbolicEqualAreaProjection::description = "equal-area";

bool HyperbolicEqualAreaProjection::projectOntoGansPlane(const Point &preimage,
                                                         GansPoint &image) {
  image = preimage * sqrt(preimage.sqr() / 4 + 1);
  // sanity check:
  // (r,0) is thus mapped to (r sqrt(r^2/4 + 1), 0) in the Gans model,
  // which is at hyperbolic distance
  // acosh(sqrt(sqr(r sqrt(r^2/4 + 1)) + 1)) = acosh(r^2/2 + 1) from the origin;
  // therefore the disk with radius r in the Euclidean plane is mapped to the
  // disk with hyperbolic area 2 pi (cosh( acosh(r^2/2 + 1) ) - 1) = pi r^2
  // (assuming Gaussian curvature -1)
  return true;
}

void HyperbolicEqualAreaProjection::projectOntoGansPlane(const Point &inPlane,
                                                         const Point &halfWidth,
                                                         GansPoint &centre,
                                                         DOUBLE &radius) {
  // just a place-holder:
  centre = GansPoint(0, 0);
  radius = INF;
}

const std::string HyperbolicEqualDistanceProjection::description =
    "equidistant";

bool HyperbolicEqualDistanceProjection::projectOntoGansPlane(
    const Point &preimage, GansPoint &image) {
  if (preimage.inOrigin())
    image = preimage;
  else {
    DOUBLE l = preimage.length();
    image = preimage * (sinh(l) / l);
  }
  return true;
}

void HyperbolicEqualDistanceProjection::projectOntoGansPlane(
    const Point &inPlane, const Point &halfWidth, GansPoint &centre,
    DOUBLE &radius) {
  // just a place-holder:
  centre = GansPoint(0, 0);
  radius = INF;
}

const std::string InvertedEuclideanDistance::description() const {
  return "inverted";
}

DOUBLE
InvertedEuclideanDistance::distanceToPointSite(const Point &p,
                                               const PointSite *s) const {
  return (p / p.sqr() - s->location / s->location.sqr()).length();
}

void InvertedEuclideanDistance::boundDistanceToPointSite(const Point &centre,
                                                         DOUBLE halfwidth,
                                                         const PointSite *s,
                                                         Range &range) const {
  range.set(0, INF);

  // map circle (centre, halfwidth*sqrt(2)) to circle in Euclidean plane
  DOUBLE r = halfwidth * SQRTTWO;
  DOUBLE dfo = centre.length();

  DOUBLE mindfo = dfo - r;
  // if mindfo = 0, then circle goes through origin, and maps
  // to a halfplane; this will not be very common, let's not bother:
  if (mindfo == 0)
    return;

  // note: if circle interior includes the origin, mindfo < 0 ...
  DOUBLE maxdfo = dfo + r;
  DOUBLE invmindfo = (DOUBLE)1.0 / maxdfo;
  DOUBLE invmaxdfo = (DOUBLE)1.0 / mindfo; // ... and invmaxdfo < 0
  DOUBLE invdfo = (invmindfo + invmaxdfo) * 0.5;
  DOUBLE invr = (invmaxdfo - invmindfo) * 0.5; // ... and invr < 0
  Point invcentre = centre * invdfo / dfo;
  Point invsite = s->location / s->location.sqr();
  DOUBLE distance = (invcentre - invsite).length();
  if (invr < 0) // circle interior includes the origin
  {
    range.lowerBound = distance + invr;
    return;
  }
  range.lowerBound = distance - invr;
  range.upperBound = distance + invr;
}

const std::string KarlsruheDistance::description() const { return "Karlsruhe"; }

DOUBLE KarlsruheDistance::distanceToPointSite(const Point &p,
                                              const PointSite *s) const {
  DOUBLE pDistanceFromOrigin = FieldRDistance().distanceToSite(p, s);
  if (pDistanceFromOrigin == 0)
    return s->distanceFromOrigin;
  if (s->distanceFromOrigin == 0)
    return pDistanceFromOrigin;
  DOUBLE angleDistance =
      safeAcos(p * s->directionFromOrigin / pDistanceFromOrigin);
  if (angleDistance > 2)
    return pDistanceFromOrigin + s->distanceFromOrigin;
  return angleDistance * std::min(pDistanceFromOrigin, s->distanceFromOrigin) +
         abs(pDistanceFromOrigin - s->distanceFromOrigin);
}

void KarlsruheDistance::boundDistanceToPointSite(const Point &centre,
                                                 DOUBLE halfwidth,
                                                 const PointSite *s,
                                                 Range &range) const {
  // use that KA/Euclidean distance <= sqrt(2)
  DOUBLE distance = distanceToPointSite(centre, s);
  range.lowerBound = std::max(distance - halfwidth * 2, (DOUBLE)0);
  range.upperBound = distance + halfwidth * 2;
}

const std::string AzimuthDistance::description() const {
  return "azimuth difference";
}

DOUBLE AzimuthDistance::distanceToPointSite(const Point &p,
                                            const PointSite *s) const {
  if (p.inOrigin() || s->location.inOrigin())
    return 0;
  return safeAcos(p * s->directionFromOrigin / p.length());
}

void AzimuthDistance::boundDistanceToPointSite(const Point &centre,
                                               DOUBLE halfwidth,
                                               const PointSite *s,
                                               Range &range) const {
  getTurnRange(centre, halfwidth, s->location, range);
}

const std::string AngleDistance::description() const { return "angular size"; }

DOUBLE AngleDistance::distanceToPolylineSite(const Point &p,
                                             const PolylineSite *s) const {
  if ((*s)[0] == p)
    return 0;
  DOUBLE angle = ((*s)[0] - p).fi();
  DOUBLE minAngle = angle;
  DOUBLE maxAngle = angle;
  for (int i = 1; i < s->size(); ++i) {
    if ((*s)[i] == p)
      return 0;
    DOUBLE prevAngle = angle;
    angle = ((*s)[i] - p).fi();
    DOUBLE dAngle = fmod(angle - prevAngle + TWOPI * i, TWOPI);
    if (dAngle < PI) {
      angle = prevAngle + dAngle;
      keepMax(maxAngle, angle);
    } else // dAngle >= PI
    {
      angle = prevAngle + (dAngle - TWOPI);
      keepMin(minAngle, angle);
    }
  }
  // if (maxAngle == minAngle) return INF; // implied by what's below
  if (maxAngle - minAngle > TWOPI)
    return 0;
  return TWOPI / (maxAngle - minAngle) - 1.0;
}

void AngleDistance::boundDistanceToPolylineSite(const Point &centre,
                                                DOUBLE halfwidth,
                                                const PolylineSite *s,
                                                Range &range) const {
  const DOUBLE halfDiameter = halfwidth * sqrt((DOUBLE)2.0);
  // minimum angle is maximum minimum angle over all segments
  // i.e. cos of std::min angle is minimum maximum cos angle over all segments
  DOUBLE minCos = 1;
  DOUBLE maxAngle = 0;
  for (int i = 1; i < s->size(); ++i) {
    // translate and rotate coordinate system such that segment lies on x-axis,
    // centred on origin; reflect such that centre of square lies in 1st
    // quadrant from there, work with a bounding square (bounding the original,
    // now rotated square) of side length halfDiameter:
    Point travel = (*s)[i] - (*s)[i - 1];
    DOUBLE length = travel.length();
    Point midPoint = ((*s)[i] + (*s)[i - 1]) / 2;
    Point relativeCentre = centre - midPoint;
    Point direction = travel / length;
    relativeCentre = Point(abs(relativeCentre * direction),
                           abs(relativeCentre * direction.rotLeft()));
    // relative segment endpoints are at (+- length/2, 0);
    Point l(-length / 2, 0);
    Point r(length / 2, 0);

    // point of range closest to origin:
    Point closestPoint(std::max(relativeCentre.x - halfDiameter, (DOUBLE)0),
                       std::max(relativeCentre.y - halfDiameter, (DOUBLE)0));
    Point farthestPoint(relativeCentre.x + halfDiameter,
                        relativeCentre.y + halfDiameter);

    // determine maximum cos angle (smallest angle) for this segment
    if (closestPoint.y > 0 || farthestPoint.x <= r.x)
      // otherwise square contains points outside of and collinear with
      // segment, so std::min angle is 0, std::max cos = 1, nothing new

      // now std::max is attained at one of the corners of the square that is
      // on the vertical edge furthest from the origin, so
      keepMin(minCos,
              safeMax(cosAngle(l, Point(farthestPoint.x, closestPoint.y), r),
                      cosAngle(l, farthestPoint, r)));

    // determine minimum cos angle (largest angle) for this segment
    DOUBLE segmentMinCos;
    if (closestPoint.x <= r.x)
    // largest-angle point is closest intersection with y-axis
    // or closest point on left edge:
    {
      segmentMinCos = cosAngle(l, closestPoint, r);
      if (isnan(segmentMinCos))
        segmentMinCos = -1;
    } else {
      // point on supporting line through left edge that is hit first
      // is tangent to smallest touching circle of l, r and that line:
      // with centre (0, cy) such that:
      // cy^2 = closestPoint.x^2 - r.x^2
      DOUBLE cy = sqrt(sqr(closestPoint.x) - sqr(r.x));
      if (cy < closestPoint.y)
        segmentMinCos = cosAngle(l, closestPoint, r);
      else if (cy < farthestPoint.y)
        segmentMinCos = cosAngle(l, Point(closestPoint.x, cy), r);
      else
        segmentMinCos = cosAngle(l, Point(closestPoint.x, farthestPoint.y), r);
    }
    maxAngle += safeAcos(segmentMinCos);
  }

  range.lowerBound = (maxAngle >= TWOPI ? 0 : TWOPI / maxAngle - 1.0);
  DOUBLE minAngle = safeAcos(minCos);
  range.upperBound = (minAngle == 0 ? INF : TWOPI / minAngle - 1.0);
}

const std::string DetourDistance::description() const { return "detour"; }

DOUBLE DetourDistance::distanceToSegmentSite(const Point &p,
                                             const SegmentSite *s) const {
  return (s->origin - p).length() + (p - s->destination).length() - s->length;
}

void DetourDistance::boundDistanceToSegmentSite(const Point &centre,
                                                DOUBLE halfwidth,
                                                const SegmentSite *s,
                                                Range &range) const {
  DOUBLE distance = distanceToSegmentSite(centre, s);
  static const DOUBLE TWOROOTTWO = sqrt(2) * 2;
  DOUBLE maxChange = halfwidth * TWOROOTTWO;
  range.lowerBound = std::max(distance - maxChange, (DOUBLE)0);
  range.upperBound = distance + maxChange;
}

const std::string DilationDistance::description() const { return "dilation"; }

DOUBLE DilationDistance::distanceToSegmentSite(const Point &p,
                                               const SegmentSite *s) const {
  return ((s->origin - p).length() + (p - s->destination).length()) *
             s->invLength -
         1.0;
}

void DilationDistance::boundDistanceToSegmentSite(const Point &centre,
                                                  DOUBLE halfwidth,
                                                  const SegmentSite *s,
                                                  Range &range) const {
  DetourDistance().boundDistanceToSegmentSite(centre, halfwidth, s, range);
  range.lowerBound *= s->invLength;
  range.upperBound *= s->invLength;
}

const std::string cSecantDistance::description() const { return "@secant"; }

DOUBLE cSecantDistance::distanceToSite(const Point &p, const Site *s) const {
  return (p.y <= 0 ? INF : p.length() / p.y);
}

void cSecantDistance::boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                          const Site *s, Range &range) const {
  Point v(abs(centre.x), centre.y);
  range.set(INF);
  if (v.y + halfwidth < 0)
    return;

  if (v.x <= halfwidth)
    range.lowerBound = 1.0;
  else {
    Point c(v.x - halfwidth, v.y + halfwidth);
    range.lowerBound = c.length() / c.y;
  }

  if (v.y - halfwidth <= 0)
    return;
  Point c(v.x + halfwidth, v.y - halfwidth);
  range.upperBound = c.length() / c.y;
}

const std::string cCatchDistance::description() const { return "@catch"; }

DOUBLE cCatchDistance::distanceToSite(const Point &p, const Site *s) const {
  return (p.y <= 0 ? INF : p * p * 0.5 / p.y);
}

void cCatchDistance::boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                         const Site *s, Range &range) const {
  Point v(abs(centre.x), centre.y);
  range.set(INF);
  if (v.y + halfwidth < 0)
    return;

  // measure is (x^2 + y^2) / y; derivative wrt y is 1 - (x/y)^2;
  // so for fixed x, std::min. at y = x

  if (v.x <= halfwidth)
    range.lowerBound = std::max(v.y - halfwidth, (DOUBLE)0) * 0.5;
  else {
    Point c;
    c.x = v.x - halfwidth;
    c.y = (c.x < v.y - halfwidth   ? v.y - halfwidth
           : c.x < v.y + halfwidth ? c.x
                                   : v.y + halfwidth);
    range.lowerBound = c.sqr() / c.y * 0.5;
  }

  if (v.y - halfwidth <= 0)
    return;
  Point c(v.x + halfwidth, v.y - halfwidth);
  Point d(v.x + halfwidth, v.y + halfwidth);
  range.upperBound = std::max(c.sqr() / c.y, d.sqr() / d.y) * 0.5;
}

cMixedCatchDistance::cMixedCatchDistance(DOUBLE _speed) : speed(_speed) {};

const std::string cMixedCatchDistance::description() const {
  std::stringstream s;
  s << "@catch@" << speed;
  return s.str();
}

DOUBLE cMixedCatchDistance::distanceToBigSite(const Point &p,
                                              const BigSite *s) const {
  if (p.inOrigin())
    return 0;

  DOUBLE sqrDistance = p.sqr();
  DOUBLE relSpeed = speed * s->invLength;
  if (relSpeed <= 1.000001 && p.y <= 0)
    return INF;
  if (0.999999 <= relSpeed && relSpeed <= 1.000001)
    return sqrDistance * 0.5 / p.y;
  ;
  DOUBLE C = sqr(relSpeed) - 1;
  DOUBLE D = sqrDistance * C + sqr(p.y);
  if (D < 0)
    return INF;
  return relSpeed * (sqrt(D) - p.y) / C;
}

void cMixedCatchDistance::boundDistanceToBigSite(const Point &centre,
                                                 DOUBLE halfwidth,
                                                 const BigSite *s,
                                                 Range &range) const {
  // just a trivial bound, checking whether the site can be caught at all:

  range.lowerBound = 0;
  range.upperBound = INF;

  // if speed of catcher exceeds that of site, then always,
  if (speed > s->length)
    return;

  // otherwise only
  // if catcher is in wedge between lines through site with opening angle
  // two times arcsin catcherspeed/sitespeed

  DOUBLE dy = sqrt(sqr(s->length / speed) - 1);

  // check if the window, is under the line with slope -dy:
  if (centre.y + halfwidth < -dy * (centre.x + halfwidth))
    range.erase();
  else
    // check if the window, is under the line with slope  dy:
    if (centre.y + halfwidth < dy * (centre.x - halfwidth))
      range.erase();

  return;
}

cPushDistance::cPushDistance(DOUBLE _acceleration)
    : acceleration(_acceleration) {};

const std::string cPushDistance::description() const {
  std::stringstream s;
  s << "@push@" << acceleration;
  return s.str();
}

DOUBLE cPushDistance::distanceToBigSite(const Point &p,
                                        const BigSite *s) const {
  // goal: find smallest t s.t. sqrt((py - v*t)^2 + px^2) <= s t^2 / 2,
  // where v is current site speed, s is acceleration
  // <=>  sqrt(((2 py/s) - (2 v/s)*t)^2 + (2 px/s)^2) <= t^2

  // scale x, y, and v with factor 2/s and rewrite:
  Point q = p * 2 / acceleration;
  DOUBLE v = s->length * 2.0 / acceleration;

  // goal: find smallest t >= 0 s.t. sqrt((qy - v*t)^2 + qx^2) <= t^2
  // that is: t^4 - v^2 t^2 + 2 qy v t - (qx^2 + qy^2) = 0,
  // that is: t^4 + b t^2 + c t + d = 0 for
  DOUBLE b = -sqr(v);
  DOUBLE c = q.y * v * 2;
  DOUBLE d = -q.sqr();

  // wikipedia solution:
  if (d == 0)
    return 0;

  // the solution below only works for c != 0; for c close to 0 there are
  // numerical issues, so let's round c to zero when p.y * v is a fraction
  // of a pixel's height and then use a solution specifically for c = 0:
  if (abs(c) < 0.0001 * options.pixelHalfWidth) {
    // if b is also small then t will now become approximately sqrt(q.length())
    // add c * sqrt(q.length()) to d to compensate for setting c to 0;
    d += c * sqrt(q.length());

    // solve t^4 + b t^2 + d = 0, i.e.
    DOUBLE minhalfb = -b / 2;
    // t^2 = minhalfb +- sqrt(sqr(minhalfb) - d)
    return sqrt(minhalfb + sqrt(sqr(minhalfb) - d));
  }

  DOUBLE minhalfq = b * b * b / 216 - b * d / 6 + c * c / 16;
  DOUBLE thirdp = -b * b / 36 - d / 3;
  DOUBLE cubethirdp = thirdp * thirdp * thirdp;
  DOUBLE disc = sqr(minhalfq) + cubethirdp;
  // note: if c = 0, then disc = (b^4/144 - b^2d/2 + d^2) (-d/27) > 0

  // let yy be a real root of 2y^3 - by^2 - 2dy + (bd-c^2/4):
  DOUBLE yy;
  if (disc < 0) {
    DOUBLE rootminthirdp = sqrt(-thirdp);
    yy = b / 6 + (DOUBLE)2 * rootminthirdp *
                     cos(safeAcos(-minhalfq / thirdp / rootminthirdp) / 3);
  } else {
    DOUBLE ww = cbrt(minhalfq + sqrt(disc));
    yy = b / 6 + ww - thirdp / ww;
  }

  // i believe i verified at some point that yy*2 - b is always non-negative in
  // our context, it only seems to get close to 0 when c is close to 0.
  DOUBLE twoyyminb = yy * 2 - b;
  DOUBLE roottwoyyminb = sqrt(twoyyminb);

  DOUBLE disc1 = -yy * 2 - b + c * 2 / roottwoyyminb;
  DOUBLE disc2 = -yy * 2 - b - c * 2 / roottwoyyminb;

  // now there are four solutions to t^4 + b t^2 + c t + d = 0:
  // t1 = -roottwoyyminb - sqrt(disc1)
  // t2 = -roottwoyyminb + sqrt(disc1)
  // t3 =  roottwoyyminb - sqrt(disc2)
  // t4 =  roottwoyyminb + sqrt(disc2)
  // we need the smallest non-negative solution
  // t1 can be ignored: it is always negative

#ifdef CONSERVATIVE
  DOUBLE minT = INF;
  DOUBLE t;
  if (disc1 >= 0) {
    t = (-roottwoyyminb + sqrt(disc1)) / 2;
    if (t >= 0)
      keepMin(minT, t);
  }
  if (disc2 >= 0) {
    DOUBLE sqrtDisc2 = sqrt(disc2);
    t = (roottwoyyminb - sqrtDisc2) / 2;
    if (t >= 0)
      keepMin(minT, t);
    t = (roottwoyyminb + sqrtDisc2) / 2;
    if (t >= 0)
      keepMin(minT, t);
  }
  return minT;
#else
  // it looks (unverified) like we need t2 if p.y > 0 and t4 if p.y < 0, but
  // this seems to create some noise, maybe due to rounding errors in disc1 and
  // disc2? it looks (unverified) like t3 is never the smallest non-negative
  // solution
  if (q.y >= 0)
    return (-roottwoyyminb + sqrt(disc1)) / 2;
  return (roottwoyyminb + sqrt(disc2)) / 2;
#endif
}

const std::string cTurnDistance::description() const { return "@turn"; }

DOUBLE cTurnDistance::distanceToSite(const Point &p, const Site *s) const {
  return atan2(abs(p.x), p.y);
}

void cTurnDistance::boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                        const Site *s, Range &range) const {
  Point direction(0, 1);
  getTurnRange(centre, halfwidth, direction, range);
}

const std::string cLeftTurnDistance::description() const {
  return "@left turn";
}

DOUBLE cLeftTurnDistance::distanceToSite(const Point &p, const Site *s) const {
  if (p.inOrigin())
    return 0;
  return atan2(p.x, -p.y) + PI;
}

void cLeftTurnDistance::boundDistanceToSite(const Point &centre,
                                            DOUBLE halfwidth, const Site *s,
                                            Range &range) const {
  // if the ray cuts the square, then bounds are [0, TWOPI)
  // otherwise the lower and upper bound are attained at corners
  // the ray cuts the square iff lowest and highest values of
  // corners are at least PI apart
  range.erase();
  Point corner[4] = {centre + Point(-halfwidth, -halfwidth),
                     centre + Point(-halfwidth, halfwidth),
                     centre + Point(halfwidth, -halfwidth),
                     centre + Point(halfwidth, halfwidth)};
  for (int i = 0; i < 4; ++i)
    range.extendTo(distanceToSite(corner[i], s));
  if (range.upperBound - range.lowerBound < PI)
    return;

  // ray cuts the square:
  range.set(0, TWOPI);
}

const std::string cDubinsDistance::description() const { return "@Dubins"; }

DOUBLE cDubinsDistance::distanceToBigSite(const Point &p,
                                          const BigSite *s) const {
  // if p is at the starting point, return 0:
  if (p.inOrigin())
    return 0;

  // if turning angle radius is zero, return Euclidean distance:
  if (s->length == 0)
    return p.length();

  // otherwise scale with inverse turning angle radius and
  // reflect such that destination is to the left
  Point v(-abs(p.x) * s->invLength, p.y * s->invLength);

  // get location relative to turning circle centre
  Point ru = v - Point(-1, 0);

  DOUBLE ruSqLength = ru * ru;
  if (ruSqLength < 1) { // within turning circle
    // get location relative to opposite turning circle centre
    ru = v - Point(1, 0);
    // get distance to opposite turning circle centre
    ruSqLength = ru * ru;
    DOUBLE ruLength = sqrt(ruSqLength);
    return (
               // right turn if destination would be on y-axis:
               safeAcos((DOUBLE)0.25 * ruLength + (DOUBLE)0.75 / ruLength) +
               // further right turn:
               atan2(ru.y, -ru.x) +
               // left turn:
               TWOPI - safeAcos((DOUBLE)1.25 - (DOUBLE)0.25 * ruSqLength)) *
           s->length; // scaled back to original dimensions
  } else {            // outside turning circle
    DOUBLE ruLength = sqrt(ruSqLength);
    return (
               // direction relative to turning circle centre
               atan2(-ru.y, -ru.x) + HALFPI +
               // additional angle because of approach from tangent point
               HALFPI - safeAcos((DOUBLE)1.0 / ruLength) +
               // straight part:
               sqrt(ruSqLength - 1)) *
           s->length; // scaled back to original dimensions
  }
}

void cDubinsDistance::boundDistanceToBigSite(const Point &centre,
                                             DOUBLE halfwidth, const BigSite *s,
                                             Range &range) const {
  // coarse bounds:
  // Euclidean distance is lower bound on Dubins distance
  // Euclidean distance + full circle is upper bound on Dubins distance
  DOUBLE distance = centre.length();
  DOUBLE radius = halfwidth * sqrt(2.0);
  range.lowerBound = std::max(distance - radius, (DOUBLE)0);
  range.upperBound = distance + radius + TWOPI * s->length;
}

#ifdef INCLUDE_USER_DISTANCES
#include "userdistances.cpp"
#endif

class TspDistance : public SegmentDistance {
public:
  virtual const std::string description() const { return "TspDistance"; }
  virtual DOUBLE distanceToSegmentSite(const Point &p,
                                       const SegmentSite *s) const {
    DOUBLE l = s->getTspTourLength();
    // std::cerr << "\rtspTourlength: "<< l << std::endl;
    return (s->origin - p).length() + (p - s->destination).length() +
           s->getTspTourLength();
  }
  void boundDistanceToSegmentSite(const Point &centre, DOUBLE halfwidth,
                                  const SegmentSite *s, Range &range) const {
    DOUBLE distance = distanceToSegmentSite(centre, s);
    static const DOUBLE TWOROOTTWO = sqrt(2) * 2;
    DOUBLE maxChange = halfwidth * TWOROOTTWO;
    range.lowerBound = std::max(distance - maxChange, (DOUBLE)0);
    range.upperBound = distance + maxChange;
  }
};

Distance *getDistanceObject(std::string dname) {
  // strip outer spaces
  int expStart = 0;
  while (expStart < dname.length() && dname[expStart] == ' ')
    ++expStart;
  int expEnd = dname.length();
  while (expEnd > 0 && dname[expEnd - 1] == ' ')
    --expEnd;
  if (expEnd <= expStart)
    Complaint(usageError) << "missing distance name";
  dname.erase(expEnd);
  dname.erase(0, expStart);

  // strip outer parenthesis, if matched
  if (dname.front() == '(' && dname.back() == ')') {
    int parenthesisDepth = 1;
    for (int i = 1; parenthesisDepth > 0 && i < dname.length() - 1; ++i) {
      switch (dname[i]) {
      case ')':
        --parenthesisDepth;
        break;
      case '(':
        ++parenthesisDepth;
        break;
      default:
        break;
      }
    }
    if (parenthesisDepth > 1)
      Complaint(usageError) << "unmatched parenthesis in distance formula";
    if (parenthesisDepth == 1) // call again without outer parenthesis
      return getDistanceObject(dname.substr(1, dname.length() - 2));
  }

  // supported operators in order of decreasing precedence:
  // (parsing need to go in opposite order: lowest first)
  // ln(...)  logarithm
  // ^  power                         from left to right
  // *  multiplication and / division from left to right
  // ~  absolute difference           from left to right
  // +  addition and - subtraction    from left to right
  // < and >  comparison indicator    from left to right
  // &  minimum                       from left to right
  // |  maximum                       from left to right
  // ?  2nd if 1st non-zero, INF oth.
  //
  // find the lowest-priority operator that is not in parenthesis:
  // tricky part: distinguish operator minus from number minus
  // states:
  //  2: after l received in state 1: expect nr minus, other chars, no operators
  //  1: after start or any operator: expect (, l, nr minus, other chars, no
  //  operators 0: (standard case)            : expect anything except a nr
  //  minus
  // -1: after )                    : expect ) or operator

  int operatorPosition = dname.length();
  int operatorPriority = 9;
  int parenthesisDepth = 0;
  int state = 1;
  for (int i = 0; i < dname.length(); ++i) {
    char c = dname[i];
    if (c == ')') {
      if (state > 1)
        Complaint(usageError) << "unexpected ) in distance formula";
      --parenthesisDepth;
      if (parenthesisDepth < 0)
        Complaint(usageError) << "unmatched parenthesis in distance formula";
      state = -1;
      continue;
    }
    if (c == '(') {
      if (state == -1 || state == 2)
        Complaint(usageError) << "unexpected ( in distance formula";
      ++parenthesisDepth;
      state = 1;
      continue;
    }
    switch (c) {
    case '?':
      if (state > 0)
        Complaint(usageError) << "unexpected " << c << " in distance formula";
      state = 1;
      if (parenthesisDepth > 0)
        continue;
      operatorPosition = i;
      operatorPriority = 1;
      break;
    case '|':
      if (state > 0)
        Complaint(usageError) << "unexpected " << c << " in distance formula";
      state = 1;
      if (parenthesisDepth > 0 || operatorPriority < 2)
        continue;
      operatorPosition = i;
      operatorPriority = 2;
      break;
    case '_':
    case '&':
      if (state > 0)
        Complaint(usageError) << "unexpected " << c << " in distance formula";
      state = 1;
      if (parenthesisDepth > 0 || operatorPriority < 3)
        continue;
      operatorPosition = i;
      operatorPriority = 3;
      break;
    case '-':
      if (state > 0)
        continue; // might be the sign of a number
    case '+':
      if (state > 0)
        Complaint(usageError) << "unexpected " << c << " in distance formula";
      state = 1;
      if (parenthesisDepth > 0 || operatorPriority < 4)
        continue;
      operatorPosition = i;
      operatorPriority = 4;
      break;
    case '>':
    case '<':
      if (state > 0)
        Complaint(usageError) << "unexpected " << c << " in distance formula";
      state = 1;
      if (parenthesisDepth > 0 || operatorPriority < 5)
        continue;
      operatorPosition = i;
      operatorPriority = 5;
      break;
    case '~':
      if (state > 0)
        Complaint(usageError) << "unexpected " << c << " in distance formula";
      state = 1;
      if (parenthesisDepth > 0 || operatorPriority < 6)
        continue;
      operatorPosition = i;
      operatorPriority = 6;
      break;
    case '*':
    case '/':
      if (state > 0)
        Complaint(usageError) << "unexpected " << c << " in distance formula";
      state = 1;
      if (parenthesisDepth > 0 || operatorPriority < 7)
        continue;
      operatorPosition = i;
      operatorPriority = 7;
      break;
    case '^':
      if (state > 0)
        Complaint(usageError) << "unexpected " << c << " in distance formula";
      state = 1;
      if (parenthesisDepth > 0 || operatorPriority < 8)
        continue;
      operatorPosition = i;
      operatorPriority = 8;
      break;
    case 'l':
      if (state == 1) {
        state = 2;
        continue;
      }
    default:
      if (state == -1)
        Complaint(usageError)
            << "read " << c
            << " while expecting one of _|+-~*/^) in distance formula";
      state = 0;
      break;
    }
  }
  if (parenthesisDepth > 0)
    Complaint(usageError) << "unmatched parenthesis in distance formula";
  if (operatorPosition < dname.length()) {
    Distance *a = getDistanceObject(dname.substr(0, operatorPosition));
    Distance *b = getDistanceObject(dname.substr(operatorPosition + 1));
    switch (dname[operatorPosition]) {
    case '?':
      return new ConditionalDistance(a, b);
    case '|':
      return new MaxDistance(a, b);
    case '_':
      return new MinDistance(a, b);
    case '&':
      return new MinDistance(a, b);
    case '<':
      return new LessDistance(a, b);
    case '>':
      return new LessDistance(b, a);
    case '+':
      return new SumDistance(a, b);
    case '-':
      return new DifferenceDistance(a, b);
    case '~':
      return new AbsoluteDifferenceDistance(a, b);
    case '*':
      return new ProductDistance(a, b);
    case '/':
      return new RatioDistance(a, b);
    case '^':
      return new PowerDistance(a, b);
    default:
      Complaint(internalError)
          << "lost operator while parsing distance formula";
    }
  }
  if (dname == "fx")
    return new FieldXDistance();
  if (dname == "fy")
    return new FieldYDistance();
  if (dname == "fr")
    return new FieldRDistance();
  if (dname == "sx")
    return new SiteXDistance();
  if (dname == "sy")
    return new SiteYDistance();
  if (dname == "sr")
    return new SiteRDistance();
  if (dname == "squared euclidean" || dname == "squared")
    return new SquaredEuclideanDistance();
  if (dname == "euclidean" || dname == "l2")
    return new EuclideanDistance();
  if (dname == "highway" || dname == "euclidean highway")
    return new EuclideanHighwayDistance();
  if (dname == "manhattan" || dname == "l1")
    return new ManhattanDistance();
  if (dname == "manhattan highway")
    return new ManhattanHighwayDistance();
  if (dname == "@triangular")
    return new cTriangularGridDistance();
  if (dname == "triangular")
    return new TranslatedDistance(new cTriangularGridDistance());
  if (dname == "tspdistance")
    return new TspDistance();
  if (dname.substr(0, 9) == "spherical") {
    if (dname[9] != '(')
      Complaint(usageError) << "missing parameters for spherical distance";
    dname.erase(0, 10);
    if (dname.back() != ')') {
      int closing = dname.find(')');
      Complaint(usageError) << "unexpected text \"" << dname.substr(closing + 1)
                            << "\" in distance formula";
    }
    dname.back() = ',';
    int comma = dname.find(',');
    std::string projection = dname.substr(0, comma);
    dname.erase(0, comma + 1);

    // aliases and default aspect ratios:
    DOUBLE aspectRatio = 1;
    if (projection == "polar azimuthal")
      projection = "stereographic";
    else if (projection == "conformal azimuthal" ||
             projection == "central azimuthal")
      projection = "gnomonic";
    else if (projection == "orthographic azimuthal")
      projection = "orthographic";
    else if (projection == "azimuthal" || projection == "lambert azimuthal" ||
             projection == "lambert azimuthal equal-area" ||
             projection == "equal-area azimuthal")
      projection = "azimuthal equal-area";
    else if (projection == "equidistant" || projection == "equal-distance" ||
             projection == "equal-distance azimuthal" ||
             projection == "azimuthal equal-distance" ||
             projection == "equidistant azimuthal")
      projection = "azimuthal equidistant";
    else
      // if (projection == "central cylindrical")
      if (projection == "equal-area cylindrical")
        projection = "cylindrical equal-area";
    // no aliases for central cylindrical
    if (projection == "orthographic cylindrical" ||
        projection == "axial cylindrical" || projection == "axial" ||
        projection == "lambert cylindrical" ||
        projection == "lambert cylindrical equal-area")
      projection = "cylindrical equal-area";
    else if (projection == "behrmann") {
      projection = "cylindrical equal-area";
      aspectRatio = 0.75;
    } else if (projection == "gall" || projection == "gall orthographic" ||
               projection == "peters") {
      projection = "cylindrical equal-area";
      aspectRatio = 0.5;
    } else
      // if (projection == "equirectangular")
      if (projection == "gall isographic") {
        projection = "equirectangular";
        aspectRatio = sqrt(0.5);
      } else if (projection == "conformal cylindrical")
        projection = "mercator";
      else if (projection == "sinusoidal")
        aspectRatio = 2;
      else if (projection == "elliptical" || projection == "mollweide" ||
               projection == "equal-area elliptical" ||
               projection == "elliptical equal-area") {
        projection = "mollweide";
        aspectRatio = 2;
      } else if (projection == "hammer")
        aspectRatio = 2;
      else if (projection == "aitoff")
        aspectRatio = 2;
      else if (projection == "stab" || projection == "stabius" ||
               projection == "1 leaf")
        projection = "werner";

    // set default radius:
    DOUBLE radius = 1;
    if (!dname.empty()) {
      int comma = dname.find(',');
      aspectRatio = strtofstop(dname.substr(0, comma));
      if (aspectRatio <= 0)
        Complaint(usageError) << "invalid aspect ratio " << aspectRatio
                              << " for spherical distance";
      dname.erase(0, comma + 1);
      if (!dname.empty()) {
        int comma = dname.find(',');
        radius = strtofstop(dname.substr(0, comma));
        if (radius <= 0 || radius > 1)
          Complaint(usageError)
              << "invalid radius " << radius << " for spherical distance";
        dname.erase(0, comma + 1);
        if (!dname.empty())
          Complaint(usageError) << "unexpected parameter \"" << dname
                                << "\" for spherical distance";
      }
    }

    if (projection == "azimuthal equal-area")
      return new SphericalDistance<
          AzimuthalProjection<AzimuthalEqualAreaProjection>>(aspectRatio,
                                                             radius);
    if (projection == "stereographic")
      return new SphericalDistance<
          AzimuthalProjection<StereographicProjection>>(aspectRatio, radius);
    if (projection == "azimuthal equidistant")
      return new SphericalDistance<AzimuthalProjection<EquidistantProjection>>(
          aspectRatio, radius);
    if (projection == "gnomonic")
      return new SphericalDistance<AzimuthalProjection<GnomonicProjection>>(
          aspectRatio, radius);
    if (projection == "orthographic")
      return new SphericalDistance<AzimuthalProjection<OrthographicProjection>>(
          aspectRatio, radius);
    if (projection == "central cylindrical")
      return new SphericalDistance<
          CylindricalProjection<CentralCylindricalProjection>>(aspectRatio,
                                                               radius);
    if (projection == "cylindrical equal-area")
      return new SphericalDistance<
          CylindricalProjection<CylindricalEqualAreaProjection>>(aspectRatio,
                                                                 radius);
    if (projection == "equirectangular")
      return new SphericalDistance<
          CylindricalProjection<EquirectangularProjection>>(aspectRatio,
                                                            radius);
    if (projection == "mercator")
      return new SphericalDistance<CylindricalProjection<MercatorProjection>>(
          aspectRatio, radius);
    if (projection == "sinusoidal")
      return new SphericalDistance<SinusoidalProjection>(aspectRatio, radius);
    if (projection == "mollweide")
      return new SphericalDistance<MollweideProjection>(aspectRatio, radius);
    if (projection == "hammer")
      return new SphericalDistance<HammerProjection>(aspectRatio, radius);
    if (projection == "aitoff")
      return new SphericalDistance<AitoffProjection>(aspectRatio, radius);
    if (projection == "werner")
      return new SphericalDistance<WernerProjection>(aspectRatio, radius);
    else
      Complaint(usageError) << "unknown projection method \"" << projection
                            << "\" for spherical distance";
  }
  if (dname.substr(0, 10) == "hyperbolic")
    return getDistanceObject(std::string("arcosh(cosh ") + dname + ")");
  if (dname.substr(0, 15) == "cosh hyperbolic") {
    if (dname[15] != '(')
      Complaint(usageError) << "missing parameter for hyperbolic distance";
    dname.erase(0, 16);
    if (dname.back() != ')') {
      int closing = dname.find(')');
      Complaint(usageError) << "unexpected text \"" << dname.substr(closing + 1)
                            << "\" in distance formula";
    }
    std::string projection = dname.substr(0, dname.length() - 1);
    if (projection == "gans")
      return new HyperbolicDistance<GansProjection>();
    if (projection == "inverted")
      return new HyperbolicDistance<InvertedGansProjection>();
    if (projection == "poincare" || projection == "poincare disk" ||
        projection == "conformal" || projection == "equal-angle")
      return new HyperbolicDistance<PoincareDiskProjection>();
    if (projection == "poincare halfplane")
      return new HyperbolicDistance<PoincareHalfplaneProjection>();
    if (projection == "klein" || projection == "klein disk" ||
        projection == "gnomonic" || projection == "equal-geodesic")
      return new HyperbolicDistance<KleinProjection>();
    if (projection == "equal-area")
      return new HyperbolicDistance<HyperbolicEqualAreaProjection>();
    if (projection == "equal-distance" || projection == "equidistant")
      return new HyperbolicDistance<HyperbolicEqualDistanceProjection>();
    else
      Complaint(usageError) << "unknown projection method \"" << projection
                            << "\" for hyperbolic distance";
  }
  if (dname == "inverted")
    return new InvertedEuclideanDistance();
  if (dname.substr(0, 10) == "translated") {
    if (dname[10] != '(')
      Complaint(usageError)
          << "missing distance measure parameter for translated distance";
    dname.erase(0, 11);
    if (dname.back() != ')') {
      int closing = dname.find(')');
      Complaint(usageError) << "unexpected text \"" << dname.substr(closing + 1)
                            << "\" in distance formula";
    }
    dname.erase(dname.length() - 1);
    return new TranslatedDistance(getDistanceObject(dname));
  }
  if (dname.substr(0, 8) == "oriented") {
    if (dname[8] != '(')
      Complaint(usageError)
          << "missing distance measure parameter for oriented distance";
    dname.erase(0, 9);
    if (dname.back() != ')') {
      int closing = dname.find(')');
      Complaint(usageError) << "unexpected text \"" << dname.substr(closing + 1)
                            << "\" in distance formula";
    }
    dname.erase(dname.length() - 1);
    return new OrientedDistance(getDistanceObject(dname));
  }
  if (dname == "dt" || dname == "azimuth difference" || dname == "azimuth")
    return new AzimuthDistance();
  if (dname == "logradius")
    return getDistanceObject("abs(ln(fr/sr))");
  if (dname == "karlsruhe")
    return new KarlsruheDistance(); // (fr~sr)+(dt&2)*(fr&sr)
  if (dname == "city")
    return getDistanceObject("sqrt(sq(dt)+sq(ln(fr/sr)))");
  if (dname == "koeln")
    return getDistanceObject("dt + abs(ln(fr/sr))");
  if (dname == "orbitout")
    return getDistanceObject("0 | 1/sr - 2/(sr+euclidean+fr)");
  if (dname == "orbitin")
    return getDistanceObject("0 | 1/fr - 2/(sr+euclidean+fr)");
  if (dname == "angular size" || dname == "angle")
    return new AngleDistance();
  if (dname == "detour")
    return new DetourDistance();
  if (dname == "dilation")
    return new DilationDistance();
  if (dname == "halfplane indicator" || dname == "semi")
    return getDistanceObject("1/oriented(0<fy)");
  if (dname == "@secant")
    return new cSecantDistance();
  if (dname.substr(0, 7) == "@catch@") {
    DOUBLE speed = strtofstop(dname.substr(7));
    if (speed <= 0)
      Complaint(usageError) << "invalid speed setting for catch distance";
    return new cMixedCatchDistance(speed);
  }
  if (dname == "@catch")
    return new cCatchDistance();
  if (dname.substr(0, 6) == "@push@") {
    DOUBLE acceleration = strtofstop(dname.substr(6));
    if (acceleration <= 0)
      Complaint(usageError) << "invalid acceleration setting for push distance";
    return new cPushDistance(acceleration);
  }
  if (dname == "@turn")
    return new cTurnDistance();
  if (dname == "@left turn" || dname == "@leftturn")
    return new cLeftTurnDistance();
  if (dname == "@dubins")
    return new cDubinsDistance();
  if (dname == "secant" || dname == "catch" || dname.substr(0, 6) == "catch@" ||
      dname.substr(0, 5) == "push@" || dname == "turn" ||
      dname == "left turn" || dname == "leftturn" || dname == "dubins")
    return new OrientedDistance(getDistanceObject(std::string("@") + dname));
  if (dname == "@lminf")
    return new cLMinDistance();
  if (dname == "std::min" || dname == "l-inf" || dname == "lminf")
    return new TranslatedDistance(new cLMinDistance());
  if (dname == "@l0")
    return new cVolumeDistance();
  if (dname == "bbox volume" || dname == "bbox" || dname == "l0")
    return new TranslatedDistance(new cVolumeDistance());
  if (dname == "@linf")
    return new cLMaxDistance();
  if (dname == "std::max" || dname == "linf")
    return new TranslatedDistance(new cLMaxDistance());
#ifdef INCLUDE_USER_DISTANCES
  MAKEUSERDISTANCE
#endif
  if (dname.substr(0, 2) == "ln")
    return new LogDistance(getDistanceObject(dname.substr(2)));
  if (dname.substr(0, 3) == "abs")
    return new AbsDistance(getDistanceObject(dname.substr(3)));
  if (dname.substr(0, 4) == "sqrt")
    return new SquareRootDistance(getDistanceObject(dname.substr(4)));
  if (dname.substr(0, 2) == "sq")
    return new SquareDistance(getDistanceObject(dname.substr(2)));
  /* untested:
    if (dname.substr(0,6) == "arccos") return new
    ArccosDistance(getDistanceObject(dname.substr(6)));
  */
  if (dname.substr(0, 6) == "arcosh")
    return new ArcoshDistance(getDistanceObject(dname.substr(6)));
  if (dname.substr(0, 2) == "@l") {
    DOUBLE exponent = strtofstop(dname.substr(2));
    if (exponent == 0)
      return new cVolumeDistance();
    else
      return new cLDistance(exponent);
  }
  if (dname[0] == 'l') {
    DOUBLE exponent = strtofstop(dname.substr(1));
    if (exponent == 0)
      return new TranslatedDistance(new cVolumeDistance());
    else
      return new TranslatedDistance(new cLDistance(exponent));
  }
  std::stringstream s(dname);
  s >> std::ws;
  if (s.eof())
    Complaint(usageError) << "missing distance name";
  DOUBLE weight;
  s >> weight >> std::ws;
  if (s.eof()) // it is indeed just a number
    return new ConstantDistance(weight);
  Complaint(usageError) << "unrecognised distance name \"" << dname << "\"";
  return NULL;
}

/*********************************************************
 * Diagram                                                *
 *********************************************************/

Element::Element() : group(-1), distance(NAN), shade(1), contour(0) {};

const short Element::softEdge = 1;
const short Element::minorContour = 2;
const short Element::majorContour = 4;
const short Element::bisector = 8;

void putContour(const short level, Element &e1) { e1.contour |= level; }

void putContour(const short level, Element &e1, Element &e2) {
  e1.contour |= level;
  e2.contour |= level;
}

/*********************************************************
 * Owner                                                  *
 *********************************************************/

Owner::Owner() : primary(-1), secondary(-1) {};

const bool Owner::operator==(const Owner &o) const {
  return (primary == o.primary && secondary == o.secondary);
}

const bool Owner::operator!=(const Owner &o) const { return !operator==(o); }

#if defined(_WIN32)
#include <fcntl.h>
#include <io.h>
static void set_stdout_binary() { _setmode(_fileno(stdout), _O_BINARY); }
#else
static void set_stdout_binary() {}
#endif
/*********************************************************
 * Main                                                   *
 *********************************************************/

std::vector<tsp_puzzle::Candidate>
vorosketch_main(const std::vector<tsp_puzzle::Node> &nodes,
                const std::vector<tsp_puzzle::Decommission> &decommissions,
                const int resolution, const bool renderImage,
                const double delta,
                const std::filesystem::path &output_path) {

  if (!std::numeric_limits<DOUBLE>::has_infinity)
    Complaint(warning) << "Number type does not support infinity. Unexpected "
                          "results may occur.";
  //  int argc = (int)(sizeof(argv) / sizeof(argv[0]));
  std::string res = std::to_string(resolution);
  std::vector<std::string> args = {"vorosketch",
                                  "-2",
                                   "-m",
                                   "tspdistance",
                                  "-r",
                                  res};
  if (renderImage) {
    args.push_back("-c");
    args.push_back("-j");
  }
  std::vector<const char *> cargs;
  cargs.reserve(args.size());
  for (auto &s : args)
    cargs.push_back(s.c_str());
  options.parse((int)cargs.size(), cargs.data());

  // options.parse(argc, argv);

  // get sites
  const int nrGroups = (int)decommissions.size();
  if (nrGroups < 1)
    Complaint(userError) << "invalid number of sites";

  std::vector<Site *> sites;
  std::vector<int> firstSite;
  sites.reserve(nrGroups);
  firstSite.reserve(nrGroups);

  for (int gid = 0; gid < nrGroups; ++gid) {
    firstSite.push_back((int)sites.size());

    const auto &d = decommissions[(size_t)gid];
    // indices must be valid for nodes
    if (d.node1 < 0 || d.node2 < 0 || d.node1 >= (int)nodes.size() ||
        d.node2 >= (int)nodes.size()) {
      Complaint(userError) << "decommission indices out of range";
    }

    // Build the segment site from nodes[node1] -> nodes[node2]
    Point a(nodes[(size_t)d.node1].x, nodes[(size_t)d.node1].y);
    Point b(nodes[(size_t)d.node2].x, nodes[(size_t)d.node2].y);
    if (!options.distance->supports(SEGMENTTYPE))
      Complaint(userError) << "TspDistance requires SEGMENTTYPE";

    auto *seg = new SegmentSite(a, b);
    // seg->setWeight(0.0);
    seg->setTspTourLength((DOUBLE)d.tsp_tour_length); // same field/meaning
    sites.push_back(seg);

    // assign group/site ids like the original input code
    const int sid = (int)sites.size() - 1;
    sites[(size_t)sid]->setId(gid, sid);
  }

  int nrSites = sites.size();
  firstSite.push_back(nrSites);

  if (options.verbose) {
    std::cerr << "\rNumber of groups: " << nrGroups << std::endl;
    std::cerr << "\rNumber of sites: " << nrSites << std::endl;
  }

  // set direction for distance comparisons
  int d = (options.order == Options::farthest ? -1 : 1);
  // scan a grid of squares to precompute viable candidates for closest sites
  if (options.verbose) {
    std::cerr << "\r> Preparing pre-scan... ";
    std::cerr.flush();
  }
  const int gridWidth = (options.width - 1) / cellWidth + 1;
  std::vector<int> prescan;
  int **candidateSites = newMatrix<int>(gridWidth, gridWidth);
  int **candidateGroups = newMatrix<int>(gridWidth, gridWidth);
  const int noGroup = nrGroups;
  const int noSite = -1; // used as a end-of-candidates marker
  long int nrComputationsScheduled = 0;

  std::vector<Range> groupDistance(nrGroups);
  std::vector<bool> hasCandidate(nrGroups);
  std::vector<Range> siteDistance(nrSites);
  DOUBLE cellHalfWidth = options.pixelHalfWidth * (cellWidth + 3);

  // compute useful candidate sites for each grid cell;
  // for each grid cell, we will push two lists on prescan:
  // first a list of candidate sites;
  // then a list of groups that have at least one candidate site;
  for (int row = 0; row < gridWidth; ++row) {
    if (options.verbose)
      reportProgress(((DOUBLE)row) / gridWidth, "> Pre-scan");
    DOUBLE y = (2.0 * row + 1.0) * cellWidth * options.pixelHalfWidth +
               options.windowOffset;
    for (int col = 0; col < gridWidth; ++col) {
      Point p((2.0 * col + 1.0) * cellWidth * options.pixelHalfWidth +
                  options.windowOffset,
              y);
      DOUBLE primUpperBound = INF;
      DOUBLE scndUpperBound = INF;
      DOUBLE maxLowerBound = 0;

      for (int i = 0; i < nrGroups; ++i) {
        groupDistance[i] = Range(INF, INF);
        hasCandidate[i] = false;
        for (int j = firstSite[i]; j < firstSite[i + 1]; ++j) {
          sites[j]->boundDistance(options.distance, p, cellHalfWidth,
                                  siteDistance[j]);
          if (siteDistance[j].isEmpty())
            continue; // skip next line: expensive with nans !?
          sites[j]->weighDistance(siteDistance[j]);
          keepMin(groupDistance[i], siteDistance[j]);
        }
        keepMin(scndUpperBound, groupDistance[i].upperBound);
        if (scndUpperBound < primUpperBound)
          std::swap(primUpperBound, scndUpperBound);
        keepMax(maxLowerBound, groupDistance[i].lowerBound);
      }
      if (options.order == Options::first)
        scndUpperBound = primUpperBound + options.equalityThreshold;

      candidateSites[row][col] = prescan.size();
      for (int j = 0; j < nrSites; ++j) {
        int i = sites[j]->groupId;
        if ((options.order != Options::farthest &&
             siteDistance[j].lowerBound <= scndUpperBound)) {
          prescan.push_back(j);
          hasCandidate[i] = true;
          ++nrComputationsScheduled;
        }
      }
      prescan.push_back(noSite);
      candidateGroups[row][col] = prescan.size();
      for (int i = 0; i < nrGroups; ++i)
        if (hasCandidate[i])
          prescan.push_back(i);
      prescan.push_back(noSite);
    }
  }
  prescan.push_back(noSite); // extra sentinel closing at the end

  if (options.verbose) {
    reportProgress(1, "> Pre-scan");
    std::cerr << "\rDistance computations eliminated by filter: "
              << std::setprecision(3)
              << 100.0 *
                     (1.0 - (DOUBLE)(cellWidth * cellWidth) *
                                nrComputationsScheduled /
                                ((long)options.width * options.width * nrSites))
              << '%' << std::endl;
  }

  // prepare the output candidates
  std::vector<tsp_puzzle::Candidate> outCandidates;
  outCandidates.reserve(options.width * options.width / 8); // heuristic

  // reserve space for two rows with full info
  Sample *samples = new Sample[2 * options.width];
  DOUBLE *distances = new DOUBLE[2 * nrGroups * options.width];
  int *layers = new int[2 * nrGroups * options.width];
  for (int i = 0; i < 2 * options.width; ++i) {
    samples[i].distance = distances;
    distances += nrGroups;
    samples[i].layer = layers;
    layers += nrGroups;
    samples[i].sunlight = 0;
  }
  Sample *scanRow = samples;
  Sample *prevRow = samples + options.width;

  // reserve space for the diagram
  Element **diagram = newMatrix<Element>(options.width + 1, options.width + 1);

  short boundaryStrength = 0;
  if (options.drawRegionBoundaries)
    boundaryStrength = Element::bisector;
  else if (options.sunlight)
    boundaryStrength = Element::softEdge;

  for (int row = 0; row < options.width; ++row) {
    if (options.verbose)
      reportProgress(((DOUBLE)row) / options.width, "> Calculating regions");
    DOUBLE y =
        (2.0 * row + 1.0) * options.pixelHalfWidth + options.windowOffset;
    int gridRow = row / cellWidth;
    for (int col = 0; col < options.width; ++col) {
      Point p((2.0 * col + 1.0) * options.pixelHalfWidth + options.windowOffset,
              y);
      int gridCol = col / cellWidth;
      Sample &s = scanRow[col];
      Owner &o = s.owner;
      Element &here = diagram[row][col];
      Element &abov = diagram[row + 1][col];
      Element &next = diagram[row][col + 1];

      o.primary = noGroup;
      o.secondary = noGroup;
      here.group = noGroup;

      bool computationNeeded = true;
      int firstCandidateGroup = prescan[candidateGroups[gridRow][gridCol]];

      if (options.drawStandardContours)
        for (int i = 0; i < nrGroups; ++i)
          s.layer[i] = NOLAYER;

      // check if there is any candidate site at all:
      if (firstCandidateGroup == noSite)
        // nobody's land, no need to check for bisectors, just continue to next
        // pixel:
        continue;

      // check if computation can be skipped because there is only one candidate
      // group:
      if (!options.distanceInfoNeeded &&
          prescan[candidateGroups[gridRow][gridCol] + 1] == noSite) {
        o.primary = prescan[candidateGroups[gridRow][gridCol]];
        here.group = o.primary;
        // do not continue to next pixel; still need to check for bisectors
      } else {
        // initialise the group distances:
        for (std::vector<int>::const_iterator gid =
                 prescan.begin() + candidateGroups[gridRow][gridCol];
             *gid != noSite; ++gid)
          s.distance[*gid] = INF; // or NAN?

        // calculate site distances, keep the best for each group
        for (std::vector<int>::const_iterator sid =
                 prescan.begin() + candidateSites[gridRow][gridCol];
             *sid != noSite; ++sid) {
          Site *site = sites[*sid];
          DOUBLE distance = site->distance(options.distance, p);
          if (isnan(distance))
            continue; // skip next line: expensive with nans !?
          site->weighDistance(distance);
          keepMin(s.distance[site->groupId], distance);
        }

        // scan group distances, calculate layers, and keep the best two
        DOUBLE primDistance = INF;
        DOUBLE scndDistance = INF;
        for (std::vector<int>::const_iterator gid =
                 prescan.begin() + candidateGroups[gridRow][gridCol];
             *gid != noSite; ++gid) {
          DOUBLE &distance = s.distance[*gid];
          int &layer = s.layer[*gid];

          layer = (options.contourInterval > 0
                       ? (int)floor((distance - options.contourDistance) /
                                    options.contourInterval)
                       : (distance >= options.contourDistance ? 0 : -1));
          // contours are drawn at any 2x2 square (indexed by its upper right
          // corner) that crosses the line; vice versa, if a cell and its left
          // neighbour are on opposite sides, then the squares indexed by that
          // cell and its upper neighbour cross the line; if a cell and its
          // lower neighbour are on opposite sides, then the squares indexed by
          // that cell and its right neighbour cross the line now check these
          // conditions for unit contours:

          // unit circle contours:
          if (options.drawStandardContours) {
            if (col > 0 && scanRow[col - 1].layer[*gid] != NOLAYER &&
                (layer < 0) != (scanRow[col - 1].layer[*gid] < 0))
              putContour(Element::majorContour, here, abov);
            if (row > 0 && prevRow[col].layer[*gid] != NOLAYER &&
                (layer < 0) != (prevRow[col].layer[*gid] < 0))
              putContour(Element::majorContour, here, next);
          }

          // record best distances
          if (distance * d < scndDistance) {
            o.secondary = *gid;
            scndDistance = distance * d;
            if (scndDistance < primDistance) {
              std::swap(o.primary, o.secondary);
              std::swap(primDistance, scndDistance);
            }
          }
        }
        // if there are still two, then lowest-numbered goes first
        if (o.secondary < o.primary) {
          std::swap(o.primary, o.secondary);
          std::swap(primDistance, scndDistance);
        }

        if (o.primary != noGroup &&
            s.distance[o.primary] <= options.cutOffDistance) {
          // choose the site whose colour to use in the diagram:
          here.group = o.primary;
          if (o.secondary != noGroup &&
#ifdef CHECKERBOARD
              (row / strokeWidth[hatchPen] + col / strokeWidth[hatchPen]) % 2 ==
                  0
#else
              ((row + col) / strokeWidth[hatchPen]) % 2 == 0
#endif
          )
            here.group = o.secondary;
          here.distance = s.distance[here.group];
        }
        // tsppatch: mark pixels that have too similar distance (we want large
        // differences)
        if (options.distance->description() == "TspDistance") {
          if (o.primary != noGroup && o.secondary != noGroup) {
            double diff = std::abs(scndDistance - primDistance);
            if (diff < (delta)) {
              // mark this pixel as black
              here.group = noGroup; // special marker group
              here.distance = 0;
              continue; // skip normal assignment logic below
            }
          }
        }
        // tsppatch
        // auto in_range = [&](int g) { return 0 <= g && g < nrGroups; };
        if (options.distance->description() == "TspDistance" &&
            o.primary != noGroup && o.secondary != noGroup &&
            here.group != noGroup) {
          // if (options.distance->description() == "TspDistance" &&
          //     in_range(o.primary) && in_range(o.secondary)) {
          tsp_puzzle::Candidate c;
          if (primDistance <= scndDistance) {
            // if (s.distance[o.primary] <= s.distance[o.secondary]) {
            c.nearest = o.primary;
            c.second = o.secondary;
          } else {
            c.nearest = o.secondary;
            c.second = o.primary;
          }
          c.x = p.x;
          c.y = p.y;
          outCandidates.push_back(c);
          // INFO: debug print
          // std::cout << "---------start------------\n";
          // std::cout << "primary" << primDistance << " secondary" <<
          // scndDistance
          //           << "\n";
          // std::cout << "nearest" << c.nearest << "\n";
          // std::cout << "seconda" << c.second << "\n";
          // std::cout << "---------end------------\n";
        }
      }
      // check the conditions for region contours (bisectors)
      if (col > 0 && o != scanRow[col - 1].owner)
        putContour(boundaryStrength, here, abov);
      if (row > 0 && o != prevRow[col].owner)
        putContour(boundaryStrength, here, next);
    }
    std::swap(scanRow, prevRow);
  }
  if (options.verbose)
    reportProgress(1, "> Calculating regions");

  // tsppatch
  if (options.generateImage) {

    bool **adjacent = newMatrix<bool>(nrGroups + 1, nrGroups + 1);
    for (int i = 0; i < nrGroups; ++i)
      for (int j = 0; j < nrGroups; ++j)
        adjacent[i][j] = false;

    RGB *colour = chooseColours(options, nrGroups, adjacent);

    // prepare canvas
    if (options.verbose) {
      std::cerr << "\r> Preparing canvas...   ";
      std::cerr.flush();
    }
    Canvas canvas(options.width, options.width);

    // draw regions
    for (int row = 0; row < options.width; ++row) {
      if (options.verbose)
        reportProgress(((DOUBLE)row) / options.width, "> Drawing regions");
      for (int col = 0; col < options.width; ++col) {
        Element &here = diagram[row][col];
        if (here.group == noGroup) {
          canvas.shade(row, col, options.colours.unclaimed);
          continue;
        }
        if (options.gradient == Options::colourGradient) {
          DOUBLE hue = pow(0.5, here.distance / options.halfBrightDistance) *
                       (options.colours.gradient.size() - 1);
          int step =
              std::min((int)hue, (int)options.colours.gradient.size() - 2);
          hue -= step;
          canvas.shade(
              row, col,
              RGB((1.0 - hue) * options.colours.gradient[step].red() +
                      hue * options.colours.gradient[step + 1].red(),
                  (1.0 - hue) * options.colours.gradient[step].green() +
                      hue * options.colours.gradient[step + 1].green(),
                  (1.0 - hue) * options.colours.gradient[step].blue() +
                      hue * options.colours.gradient[step + 1].blue()));
        } else
          canvas.shade(row, col, colour[here.group]);
        if (options.shadeInnerAreas && here.distance <= options.contourDistance)
          canvas.shade(row, col, 0.7529);
        if (options.gradient == Options::brightnessGradient)
          canvas.shade(row, col,
                       pow(0.5, here.distance / options.halfBrightDistance) *
                           1.14);
        // sun shading:
        canvas.shade(row, col, here.shade);
      }
    }

    if (options.verbose)
      reportProgress(1, "> Drawing regions");

    // draw sites
    if (options.drawSites) {
      if (options.verbose) {
        std::cerr << "\r> Drawing sites...";
        std::cerr.flush();
      }

      // draw the sites
      for (int i = 0; i < nrGroups; ++i) {
        if (options.colours.sites.size() > 1 || options.arrowLength > 0)
          for (int j = firstSite[i]; j < firstSite[i + 1]; ++j)
            sites[j]->drawOutline(canvas, options.colours.siteOutline);

        for (int j = firstSite[i]; j < firstSite[i + 1]; ++j) {
          if (options.colours.sites.size() == 1 || options.unicolourSites)
            sites[j]->drawFill(canvas, options.colours.site);
          else
            sites[j]->drawFill(canvas, colour[i]);
        }
      }
    }
    if (options.verbose) {
      std::cerr << "\r> Writing bitmap file...";
      std::cerr.flush();
    }

    // output bitmap
    // BitmapFile bmp(std::cout, options.width, options.width);
    // for (int row = 0; row < options.width; ++row)
    //   for (int col = 0; col < options.width; ++col)
    //     bmp.writePixel(canvas(row, col));
    const char *kFilename = "voronoi.bmp";
    std::filesystem::path fullPath = output_path / kFilename;
    std::ofstream out(fullPath, std::ios::binary);
    if (!out)
      Complaint(userError) << "could not open \"" << kFilename
                           << "\" for writing";
    BitmapFile bmp(out, options.width, options.width);
    for (int row = 0; row < options.width; ++row) {
      for (int col = 0; col < options.width; ++col) {
        bmp.writePixel(canvas(row, col));
      }
    }
    deleteMatrix<bool>(adjacent);
    delete[] colour;
  }
  // free Site* objects
  for (auto *s : sites)
    delete s;

  // free matrices
  deleteMatrix<Element>(diagram);
  deleteMatrix<int>(candidateSites);
  deleteMatrix<int>(candidateGroups);

  // free per-row buffers
  if (samples) {
    DOUBLE *distances_base = samples[0].distance;
    int *layers_base = samples[0].layer;
    delete[] distances_base;
    delete[] layers_base;
    delete[] samples;
  }
  return outCandidates;
}
} // namespace vorosketch
