#pragma once

/*********************************************************
 * Includes                                               *
 *********************************************************/

#include "candidate.h"
#include "decommission.h"
#include "node.h"
#include <cmath>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
// using std::string;
// using std::stringstream;
// using std::cerr;
// using std::cin;
// using std::cout;
// using std::endl;
// using std::istream;
// using std::ostream;
// using std::max;
// using std::min;
// using std::swap;

namespace vorosketch {

inline const char *vorosketchInfo =

    /**********************************************************************/

    "Vorosketch v0.37 BETA by Herman Haverkort, 22 March 2023";

/***********************************************************************

Copyright 2023 Herman Johannes Haverkort

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

New versions of vorosketch.cpp may be available from
http://herman.haverkort.net.


************************************************************************

WHAT IS VOROSKETCH?

Vorosketch is a simple tool to sketch small Voronoi diagrams as a bitmap.
Use it wisely. It is not designed for speed or for accuracy: it may miss
features of the diagram that are narrower than a pixel. But it is easy
to adapt to different distance measures regardless of whether we
understand the geometry of the bisectors yet.


************************************************************************

SUPPORTED DISTANCE MEASURES

Below are the supported definitions for d(p,q), where p is any point in
the plane, and q is a site. To select the distance measure named xxx,
write -m xxx on the command line. One may also select weighted
combinations, e.g. -m a*xxx+b*yyy combines xxx and yyy with coefficients
a and b.

For point or polyline sites:

  euclidean   minimum euclidean distance from p to any point of q

  squared     square of the above

  manhattan   minimum L1-distance from p to any point of q

For point sites only:

  triangular  minimum distance in a triangular grid

  spherical   angular distance on the unit sphere, projected onto the
              plane by one of various projections

  hyperbolic  distance in hyperbolic space, projected using one of
              various projections

  inverted    distance in the inverted Euclidean plane

  Lx          (where x is a real number other than zero) minimum
              Lx-distance from p to q

  min         minimum of distance in x and distance in y (L-inf)

  bbox        product of distance in x and distance in y

  max         maximum of distance in x and distance in y (Linf)

  highway     minimum travel time from p to q when higher speed of
              travel is possible on designated line segments

  manhattan highway    same when travel time outside designated the line
              segments is determined by the L1-distance

  karlsruhe   minimum distance from p to q when travelling only along
              rays from the origin and circles centred on the origin

  city        minimum travel time from p to q when speed is linearly
              proportional to the distance from the centre

  azimuth     difference between the azimuth coordinates modulo 2pi with
              respect to the origin

  logradius   logarithm of distance from the origin

  koeln       shorthand for azimuth+logradius: minimum travel time from
              p to q when travelling only along rays from the origin and
              circles centred on the origin, while speed is linearly
              proportional to the distance from the centre

  orbitout    1/|q| - 2/(|q|+|p-q|+|p|)
              this is, modulo constant factors, the minimum kinetic
              energy which we would have to give an object at q in order
              to reach p, subject to gravitation towards the origin
              (Why? The speed at q in orbit around the origin is
              proportional to the square root of 1/|q| - 1/a, where a
              is the major axis of the elliptical orbit. Since p has to
              be on that orbit, too, we get that 2a is the length of the
              path opfqo, where o is the origin and f is the other focal
              point of the elliptical orbit. To minimise 2a, and thus,
              the necessary speed at q, we put f on the segment pq.)

  orbitin     1/|p| - 2/(|p|+|q-p|+|q|)
              this is, modulo constant factors, the minimum kinetic
              energy which we would have to give an object at p in order
              to reach q, subject to gravitation towards the origin
              (note that for any given p, the site q that minimises this
              is simply the site that minimises |q-p| with an additive
              weight of |q|)

For polyline sites only:

  angle       -1 + 2pi / smallest opening angle of any wedge with apex
              at p that includes q. This is effectively: how small q
              looks as seen from p

  detour      the distance to a line segment q1q2 is |q1p|+|pq2|-|q1q2|

  dilation    the distance to a line segment q1q2 is
              (|q1p|+|pq2|-|q1q2|)/|q1q2|

For directed-point or rooted-vector sites (points q with direction
towards a point q', and possibly a magnitude):

  secant      secant of angle between qq' and qp (infinite if negative)

  catch       distance to q times secant divided by two

  catch@x     unit circles around the sites move as they grow: whenever
              the radius increases by x, the centre moves by the
              specified vector. x must be positive.

  push@x      time it takes for a site with given initial direction and
              speed to reach a point when traveling with acceleration x

  turn        minimum angle between qq' and qp

  leftturn    minimum counterclockwise angle from qq' to qp

  dubins      minimum length of a path from q to p that starts in
              direction q' and has curvature radius at most |qq'|

To include additional user-defined distance measures from
userdistances.cpp, compile with the -D INCLUDE_USER_DISTANCES option.


***********************************************************************/
inline const char *vorosketchFileFormat =

    "INPUT AND OUTPUT FORMAT                                                \n\
                                                                        \n\
The sites are read from a text file on standard input that contains a   \n\
sequence of numbers separated by spaces or line breaks.                 \n\
                                                                        \n\
The first number is the number of sites. Then there is a list of numbers\n\
for each site: first the number of points (1 for point sites, 2 for     \n\
directed-point or rooted-vector sites, 2 or more for polyline sites);   \n\
then for each point the x and y coordinate; and finally the weight of   \n\
the site (for unweighted Voronoi Diagrams, just set the weight to 0).   \n\
Groups of sites can be specified by writing the number of points of each\n\
site, except the last, as a negative number.                            \n\
                                                                        \n\
If the highway distance is used, the highways must be specified after   \n\
the sites in the same format (with weights interpreted as speed)        \n\
                                                                        \n\
The output is a bitmap in .bmp-format on standard output.               \n";

/***********************************************************************

WEIGHTED DISTANCES

By default, the specified site weights are subtracted from the
calculated distances. If the command line includes the -d option, then
the distances are divided by the specified weights instead. Thus, we
can create, among others, the following types of Voronoi diagrams:

additively weighted:         euclidean
multiplicatively weighted:   euclidean -d
power diagram:               squared

Note that definitions in the literature may differ with respect to
whether weights are added or subtracted, whether they are multiplied or
divided by, and whether they are squared or not. So be sure to input
the weights in the file in such a way that you get the intended
result.


************************************************************************

WHAT IF THERE ARE REGIONS OF NON-ZERO AREA, IN WHICH MULTIPLE SITES ARE
AT THE SAME DISTANCE?

The *two* sites that are listed in the input file first win. For this
purpose, the sites have to be at exactly the same calculated distance.
To account for rounding errors, use the -e c option to regard
distances as equal if they do not differ more than c times a pixel's
width.

In case of polyline sites, rather than identifying regions that are
closest to an endpoint shared by two line segments, one may want to
divide those regions by the angular bisector of those segments. This
can be achieved with the -s c options, which shortens the first and the
last segment of each site by c times a pixel's width.


***********************************************************************/
inline const char *vorosketchOptions =

    "OPTIONS                                                                \n\
                                                                        \n\
-m xxx   use distance measure xxx. The following measures are supported:\n\
         euclidean (default), squared, manhattan, triangular,           \n\
         spherical(p,a,r), hyperbolic(p) (see below for parameters),    \n\
         inverted, @Lx (for minf, inf, or any real number x), karlsruhe,\n\
         city, azimuth, logradius, koeln, highway, manhattan highway,   \n\
         orbitout, orbitin, angle, detour, dilation, @secant, @catch,   \n\
         @catch@x (for a positive speed x), @push@x (for a positive     \n\
         acceleration x), @turn, @leftturn, @dubins.                    \n\
         \n\
         translated(x) applies x after translating the coordinate system\n\
         such that the site is in the origin; oriented(x) also rotates  \n\
         such that the site points up. Measures that start with @ are   \n\
         defined for a canonical site in the origin, pointing up. Use   \n\
         these without @ to apply translated or oriented as appropriate.\n\
         \n\
         Spherical distance (distance on the unit sphere) requires      \n\
         projection p (\"aitoff\", \"azimuthal equal-area\", \"behrmann\",    \n\
         \"central cylindrical\", \"equidistant\", \"equirectangular\",       \n\
         \"gall\", \"gall isographic\", \"gnomonic\", \"hammer\",             \n\
         \"lambert cylindrical\", \"mercator\", \"mollweide\", \"orthographic\",\n\
         \"peters\", \"sinusoidal\", \"stereographic\", or \"werner\"), and,    \n\
         optionally, aspect ratio a, and radius r of the part of the    \n\
         sphere covered (1 = PI = all).\n\
         Hyperbolic distance requires projection p (\"Gans\", \"inverted\", \n\
         \"poincare disk\", \"poincare halfplane\", \"klein\", \"equal-area\",  \n\
         or \"equidistant\").\n\
         \n\
         Combinations can be made using standard arithmetic with the    \n\
         symbols ln sq sqrt abs ^ * / + - ~ & | < ? ( ) fx fy fr sx sy  \n\
         sr, where x~y = abs(x-y), x&y = min(x,y), x|y = max(x,y),      \n\
         x<y = 1 if x<y and 0 otherwise, x?y = y if x is non-zero and   \n\
         infinite otherwise, fx, fy are the coordinates of a point the  \n\
         plane; fr its distance from the origin; sx, sy, sr the         \n\
         coordinates and distance of a point site. For example:         \n\
         \"oriented(0<fy?@l2)\" produces a so-called semi-Voronoi       \n\
         diagram for directed-point sites (quotes protect the formula   \n\
         from the shell).                                               \n\
-d       divide by weights instead of subtracting them                  \n\
-e c     set distance equality threshold to c (default: 0)              \n\
-s c     shorten polyline sites by c times a pixel's width (default: 0) \n\
-2       render 2nd-order Voronoi diagram (cannot be combined with -e)  \n\
-n       render next-closest-site diagram (cannot be combined with -e)  \n\
-f       render farthest-site Voronoi diagram                           \n\
-a       anonymous: do not draw the sites                               \n\
-b       draw region boundaries (default when -c, -p, -z not selected)  \n\
-c       draw coloured sites and regions, using 19 colours from Sasha   \n\
         Trubetskoy's list of 20 simple, distinct colours               \n\
-p zzz   draw coloured sites and regions using the palette named zzz,   \n\
         where zzz is one of Trubetskoy-Default, Trubetskoy-Original,   \n\
         Trubetskoy-Modified, Grey, Stone, BrightStone, Flat, Bright,   \n\
         or of the form filename:palettename                            \n\
-h       try to give adjacent regions contrasting colours (experimental)\n\
-l       try to give adjacent regions similar colours (experimental)    \n\
-x       give regions with few neighbours pale colours (experimental)   \n\
-*       mark all sites with the same symbol in the same colour         \n\
-u       draw distance contour line at standard distance from each site \n\
         (0 for subtractively, 1 for divisively weighted diagrams)      \n\
-o       omit shading of regions within standard distance from their    \n\
         site (in closest-site and next-closest-site diagrams)          \n\
         (even if their are no such regions, -o can save some time)     \n\
-i x     draw contour lines at intervals of x units                     \n\
-5       emphasize every 5th contour line                               \n\
-g x     shading by distance, 50% brightness at distance x              \n\
-k x     colouring by distance, middle colour at distance x             \n\
-z       shading by sunlight based on distance gradient                 \n\
-t x     blacken areas at distance more than x from their site          \n\
-r n     set output resolution to n x n pixels (default: 2000)          \n\
-+       mark the origin                                                \n\
-v       verbose: show option settings and progress indicator on        \n\
         standard error output                                          \n\
-w x     clip diagram to [-x,x] X [-x,x] (default: [-1,1] X [-1,1])     \n\
-@ n     use n random unweighted sites instead of reading sites from    \n\
         standard input                                                 \n\
-# n     use n predefined spherical or hyperbolic point sites instead of\n\
         reading sites from the input                                   \n\
         (n from 2, 4, 6, 8, 20, 24, 36, 62 for spherical sites;        \n\
         n from 25, 50, 79 for hyperbolic sites)                        \n\
-?       show how to use this tool                                      \n";

// unused so far: -j, -k, -q , -y

/***********************************************************************

HEADERS


**********************************************************
* Tunable constants etc.                                 *
*********************************************************/

typedef long double DOUBLE;

const int nrPens = 5;

// stroke widths in pixels; must be even, because strokes
// are centered on pixel corners, not pixel centres:
const int strokeWidth[nrPens] = {2, 4, 6, 8, 12};

const int contourPen = 0;
const int bisectorPen = 1;
const int sitePen = 3;
const int hatchPen = 4;
const int siteOutlinePen = 4;
const int arrowPen = 1;
const int arrowHeadPen = 2;
const int markOutlinePen = 2;
const int softBisectorPen = 1;

// maximum number of sites (arbitrary); this is only to
// catch obvious mistakes in the input that would cause the
// program to take forever before discovered
const int MAXNRGROUPS = 20000;
const int MAXRESOLUTION = 20000;

// uncomment the following line for checkerboard pattern instead
// of diagonal hatching in shared regions:
#define CHECKERBOARD

// length of progress indicator:
const int progressUnits = 40;

// size of grid for pre-scan
const int cellWidth = 20;

const DOUBLE INF = std::numeric_limits<DOUBLE>::infinity();
const int NOLAYER = std::numeric_limits<int>::min();

/*********************************************************
 * Custom structs for Incremental puzzle		 *
 *********************************************************/
struct PixelInfo {
  int closest; // group id of closest site
  int second;  // group id of second-closest site
};

struct RenderResult {
  int width;
  int height;
  std::vector<PixelInfo> pixels;
};

std::vector<tsp_puzzle::Candidate>
vorosketch_main(const std::vector<tsp_puzzle::Node> &nodes,
                const std::vector<tsp_puzzle::Decommission> &decommissions,
                const int resolution, const bool renderImage,
                const double delta,
                const std::filesystem::path &output_path
              );

/*********************************************************
 * General tools headers                                  *
 *********************************************************/

template <class T> std::vector<T> vectorOfTwo(T first, T second);

template <class T> T **newMatrix(const long m, const long n);

template <class T> void deleteMatrix(T **A);

template <class T> T ***newMatrix(const long l, const long m, const long n);

template <class T> void deleteMatrix(T ***A);

DOUBLE &keepMin(DOUBLE &x, const DOUBLE y);

DOUBLE &keepMax(DOUBLE &x, const DOUBLE y);

DOUBLE safeMin(const DOUBLE x, const DOUBLE y);

DOUBLE safeMax(const DOUBLE x, const DOUBLE y);

bool nonDecreasing(const DOUBLE x, const DOUBLE y, const DOUBLE z);

bool xyInDifferentBracketsOfz(const int x, const int y, const int z);

struct Range {
  DOUBLE lowerBound;
  DOUBLE upperBound;
  Range(const DOUBLE _lowerBound = NAN, const DOUBLE _upperBound = NAN);
  Range &operator+=(const Range &r);
  Range &operator-=(const Range &r);
  Range operator-() const;
  Range &erase();
  Range &extendTo(const DOUBLE x);
  bool contains(const DOUBLE x) const;
  bool isEmpty() const;
  Range &set(const DOUBLE x);
  Range &set(const DOUBLE l, const DOUBLE u);
  bool is(const DOUBLE l, const DOUBLE u) const;
  DOUBLE size() const;
  DOUBLE mean() const;
};

Range &keepMin(Range &x, const Range &y);

DOUBLE rnd(const DOUBLE atLeast = 0, const DOUBLE lessThan = 1);

DOUBLE sqr(const DOUBLE x);

DOUBLE safeAcos(const DOUBLE x);

DOUBLE safeAsin(const DOUBLE x);

/*********************************************************
 * Input/output headers                                   *
 *********************************************************/

enum ErrorLevel { warning, userError, usageError, internalError };

class Complaint
// an object to which an error message can be witten with <<.
// when the object is destroyed, the error message is produced
// and, if it is not a mere warning, the program halts.
{
private:
  static const char *errorLabel[];
  std::stringstream complaint;
  ErrorLevel errorLevel;

public:
  Complaint(ErrorLevel _errorLevel = userError);
  Complaint(ErrorLevel _errorLevel, const std::string &expected,
            const std::string &read);
  ~Complaint();
  template <typename T> Complaint &operator<<(const T &t);
};

void makeLowerCase(std::string &s);

// convert a string to a number, verifying that the string
// does not contain any further content:
DOUBLE strtofstop(const std::string &str);
int strtoistop(const std::string &str);

void reportProgress(DOUBLE x, std::string description);

class RGB {
private:
  int mRed;
  int mGreen;
  int mBlue;

public:
  RGB(const int _red, const int _green, const int _blue);
  RGB(const double _grey = 1);
  const int red() const;
  const int green() const;
  const int blue() const;
  RGB &operator*=(const RGB &colour);
  RGB operator*(const DOUBLE value) const;
  RGB &operator*=(const DOUBLE value);
  void limit();
  static RGB black;
  static RGB white;
  friend std::istream &operator>>(std::istream &s, RGB &rgb);
};

std::istream &operator>>(std::istream &s, RGB &rgb);

// compute a measure for the contrast between two colours:
DOUBLE contrast(const RGB &a, const RGB &b);

// use some heuristic algorithm to assign colours to groups based on
// the chosen options and the adjacency matrix of the groups' regions:
class Options;
RGB *chooseColours(Options &options, const int nrGroups, bool **adjacent);

class BitmapFile {
private:
  std::ostream &outputStream;
  const int width;
  const int padding;
  int samplesYetToCome;
  void writeInteger(int i, const int bytes) {
    outputStream.write((char *)&i, bytes);
  }

public:
  BitmapFile(std::ostream &_outputStream, const int _width, const int height);
  BitmapFile &writePixel(RGB colour);
  ~BitmapFile();
};

/*********************************************************
 * Geometry headers                                       *
 *********************************************************/

const DOUBLE PI = 3.141592653589793;
const DOUBLE TWOPI = PI * 2.0;
const DOUBLE HALFPI = PI * 0.5;

struct Point {
  DOUBLE x;
  DOUBLE y;
  Point(const DOUBLE _x = 0, const DOUBLE _y = 0);
  bool operator==(const Point &p) const;
  bool operator!=(const Point &p) const;
  bool inOrigin() const;
  Point normalised(DOUBLE targetLength = 1.0) const;
  Point operator+(const Point &p) const;
  Point operator-(const Point &p) const;
  Point operator*(const DOUBLE scalar) const;
  Point &operator*=(const DOUBLE scalar);
  Point operator/(const DOUBLE scalar) const;
  Point &operator/=(const DOUBLE scalar);
  DOUBLE operator*(const Point &p) const;
  DOUBLE sqr() const;
  DOUBLE length() const; // length/distance from origin
  DOUBLE fi() const;     // direction from origin
  DOUBLE ySlope() const; // dy/dx
  DOUBLE xSlope() const; // dx/dy
  Point rotLeft() const;
  static constexpr DOUBLE EPS = 1e-9;
  friend std::istream &operator>>(std::istream &is, Point &p);
  bool operator<(const Point &b) const {
    if (fabs(x - b.x) > EPS)
      return x < b.x;
    if (fabs(y - b.y) > EPS)
      return y < b.y;
    return false;
  }
};

std::istream &operator>>(std::istream &is, Point &p);

Point min(const std::vector<Point> &points);

Point max(const std::vector<Point> &points);

DOUBLE cosAngle(const Point &p1, const Point &p2, const Point &p3);

void getTurnRange(Point &centre, DOUBLE halfwidth, const Point &direction,
                  Range &range);

DOUBLE safeCosXY1WithSun(const DOUBLE x, const DOUBLE y);
// computes cosine of vector (x,y,1) and unit-length sun vector
// -1 if (x,y,1).v = -inf;
//  0 if (x,y,1).v is not a number;
//  1 if (x,y,1).v =  inf

struct SphericalPoint {
  DOUBLE x;
  DOUBLE y;
  DOUBLE z;

public:
  SphericalPoint();
  SphericalPoint(const DOUBLE _x, const DOUBLE _y, const DOUBLE _z);
  SphericalPoint(const DOUBLE latitude, const DOUBLE longitude);
  DOUBLE operator*(const SphericalPoint &p) const;
  void stretchHemisphereToSphere();
};

void getCentreOfSphericalRectangle(const Range &height, const Range &longitude,
                                   SphericalPoint &centre, DOUBLE &radius);
// ! not tight if longitude range exceeds pi

struct GansPoint {
  DOUBLE x;
  DOUBLE y;
  GansPoint(const DOUBLE _x = 0, const DOUBLE _y = 0);
  GansPoint &operator=(const Point &p);
};

DOUBLE coshdistance(const GansPoint &p, const GansPoint &q);

/*********************************************************
 * Options headers                                        *
 *********************************************************/

struct Colours {
  std::vector<RGB> sites;
  std::vector<RGB> gradient;
  RGB unclaimed;
  RGB softEdgeShade;
  RGB bisector;
  RGB majorContour;
  RGB minorContourShade;
  RGB obstacle;
  RGB highway;
  RGB highwayOutline;
  RGB site;
  RGB siteOutline;
  RGB origin;
  RGB originOutline;
  RGB node;
  RGB grid;
  Colours();
  void read(std::istream &repo, std::string name);
};

struct OptionSyntax {
  int rank;
  std::string id;
  bool requiresArgument;
  bool multipleAllowed;
  OptionSyntax(int _rank, std::string _id, bool _requiresValue = false,
               bool _multipleAllowed = false);
};

class Distance;

class Options {
public:
  Distance *distance;
  bool useEuclideanHighways;
  bool useManhattanHighways;
  bool useCompositeDistance;
  enum { subtractive, divisive } weighting;
  DOUBLE equalityThreshold;
  DOUBLE segmentShortening;
  enum { first, second, nextclosest, farthest } order;
  bool drawSites;
  bool drawRegionBoundaries;
  DOUBLE arrowLength;
  Colours colours;
  enum { inputOrder, highContrast, lowContrast } colourAssignment;
  bool fadeByDegree;
  bool unicolourSites;
  bool drawStandardContours;
  DOUBLE contourInterval;
  bool shadeInnerAreas;
  DOUBLE contourDistance;
  bool emphasizeFifth;
  DOUBLE halfBrightDistance;
  enum { noGradient, brightnessGradient, colourGradient } gradient;
  bool sunlight;
  DOUBLE cutOffDistance;
  bool distanceInfoNeeded;
  int width;
  bool markOrigin;
  DOUBLE windowWidth;
  DOUBLE windowOffset;
  DOUBLE pixelHalfWidth;
  DOUBLE pixelsPerUnit;
  int randomSites;
  bool noSegmentSites;
  int predefinedSites;
  bool verbose;
  bool generateImage;

  Options();
  void parse(const int argc, const char *argv[]);
  void report(std::ostream &s) const;
  bool lineWork() const;
  static void usage();

private:
  static const OptionSyntax syntax[];
};

/*********************************************************
 * Canvas headers                                         *
 *********************************************************/

class Canvas {
private:
  const int nrRows;
  const int nrCols;
  RGB *pixels;
  RGB **rows;
  int penSize[nrPens];
  int *penMasks[nrPens];

public:
  Canvas(const int _nrRows, const int _nrCols);
  const RGB &operator()(const int row, const int col) const;
  template <class T> void shade(const int row, const int col, const T &value);
  void draw(const int row, const int col, const int pen, const RGB &colour);
  template <class T>
  void shade(const int row, const int col, const int pen, const T &value);
  void draw(const Point &p, const Point &q, const int pen, const RGB &colour);
};

/*********************************************************
 * Site headers                                           *
 *********************************************************/

typedef unsigned short SiteTypes;
const SiteTypes HYPERBOLICPOINTTYPE = 256;
const SiteTypes SPHERICALPOINTTYPE = 128;
const SiteTypes SEGMENTTYPE = 64;
const SiteTypes POLYLINETYPE = 32 | SEGMENTTYPE;
const SiteTypes ROOTEDVECTORTYPE = 16;
const SiteTypes BIGTYPE = 8 | ROOTEDVECTORTYPE;
const SiteTypes DIRECTEDPOINTTYPE = 4 | ROOTEDVECTORTYPE;
const SiteTypes POINTTYPE = 2 | DIRECTEDPOINTTYPE;
const SiteTypes ALLTYPES = 1 | POINTTYPE | BIGTYPE | POLYLINETYPE |
                           SPHERICALPOINTTYPE | HYPERBOLICPOINTTYPE;

class Site {
public:
  int groupId;
  int siteId;

private:
  DOUBLE multiplicativeWeight;
  DOUBLE subtractiveWeight;

public:
  Site();
  Site &setId(int _groupId, int _siteId);
  static void verifyWeight(const DOUBLE weight);
  Site &setDivisiveWeight(DOUBLE weight);
  Site &setSubtractiveWeight(DOUBLE weight);
  Site &setWeight(DOUBLE weight);
  Site &copyWeight(const Site *s);
  DOUBLE divisiveWeight() const; // only for Highway Map!
  void weighDistance(DOUBLE &distance) const;
  void weighDistance(Range &range) const;
  virtual DOUBLE distance(const Distance *measure, const Point &p) const;
  virtual void boundDistance(const Distance *measure, const Point &centre,
                             DOUBLE halfwidth, Range &range) const;
  virtual void drawOutline(Canvas &canvas, const RGB &colour) const;
  virtual void drawFill(Canvas &canvas, const RGB &colour) const;
  virtual void setTspTourLength(DOUBLE) {}
  virtual DOUBLE getTspTourLength() const { return 0.0; }
  virtual Point getOrigin() const {
    throw std::runtime_error("Not Segmentsite, no origin");
  }
  virtual Point getDestination() const {
    throw std::runtime_error("Not Segmentsite, no destination");
  }
};

class PointSite : virtual public Site {
public:
  const Point location;
  const DOUBLE distanceFromOrigin;
  const Point directionFromOrigin; // as unit vector; (NAN, NAN) if distance 0
  PointSite(const Point &_location);
  static PointSite *random();
  virtual DOUBLE distance(const Distance *measure, const Point &p) const;
  virtual void boundDistance(const Distance *measure, const Point &centre,
                             DOUBLE halfwidth, Range &range) const;
  virtual void drawOutline(Canvas &canvas, const RGB &colour) const;
  virtual void drawFill(Canvas &canvas, const RGB &colour) const;
};

class SphericalPointSite : virtual public Site {
public:
  const SphericalPoint sphericalLocation;
  SphericalPointSite(const SphericalPoint &_sphericalLocation);
  static SphericalPointSite *random();
  virtual DOUBLE distance(const Distance *measure, const Point &p) const;
  virtual void boundDistance(const Distance *measure, const Point &centre,
                             DOUBLE halfwidth, Range &range) const;
  virtual void drawOutline(Canvas &canvas, const RGB &colour) const;
  virtual void drawFill(Canvas &canvas, const RGB &colour) const;
};

class HyperbolicPointSite : virtual public Site {
public:
  const GansPoint hyperbolicLocation;
  HyperbolicPointSite(const GansPoint &_hyperbolicLocation);
  static HyperbolicPointSite *random();
  virtual DOUBLE distance(const Distance *measure, const Point &p) const;
  virtual void boundDistance(const Distance *measure, const Point &centre,
                             DOUBLE halfwidth, Range &range) const;
  virtual void drawOutline(Canvas &canvas, const RGB &colour) const;
  virtual void drawFill(Canvas &canvas, const RGB &colour) const;
};

class BigSite : virtual public Site {
public:
  const DOUBLE length;
  const DOUBLE invLength;
  BigSite(const DOUBLE _length);
  virtual DOUBLE distance(const Distance *measure, const Point &p) const;
  virtual void boundDistance(const Distance *measure, const Point &centre,
                             DOUBLE halfwidth, Range &range) const;
};

class DirectedPointSite : virtual public PointSite {
public:
  const Point direction; // as unit vector
  const Point normal;    // direction.rotLeft()
  DirectedPointSite(const Point &_location, const Point &_vector);
  static DirectedPointSite *random();
  virtual DOUBLE distance(const Distance *measure, const Point &p) const;
  virtual void boundDistance(const Distance *measure, const Point &centre,
                             DOUBLE halfwidth, Range &range) const;
  virtual void drawOutline(Canvas &canvas, const RGB &colour) const;
};

class RootedVectorSite : virtual public DirectedPointSite,
                         virtual public BigSite {
public:
  RootedVectorSite(const Point &_location, const Point &_vector);
  static RootedVectorSite *random(const DOUBLE scale);
  virtual DOUBLE distance(const Distance *measure, const Point &p) const;
  virtual void boundDistance(const Distance *measure, const Point &centre,
                             DOUBLE halfwidth, Range &range) const;
  virtual void drawOutline(Canvas &canvas, const RGB &colour) const;
};

class PolylineSite : public Site {
protected:
  std::vector<Point> vertices;

public:
  const Point bboxMin;
  const Point bboxMax;
  int size() const;
  const Point &operator[](const int i) const;
  PolylineSite(const std::vector<Point> &points = std::vector<Point>(0));
  static PolylineSite *random();
  virtual DOUBLE distance(const Distance *measure, const Point &p) const;
  virtual void boundDistance(const Distance *measure, const Point &centre,
                             DOUBLE halfwidth, Range &range) const;
  virtual void drawOutline(Canvas &canvas, const RGB &colour) const;
  virtual void drawFill(Canvas &canvas, const RGB &colour) const;
};

class SegmentSite : public PolylineSite {
public:
  const Point direction; // as unit vector
  const Point normal;    // direction.rotLeft
  const DOUBLE slope;
  const DOUBLE invSlope;
  const DOUBLE length;
  const DOUBLE invLength;
  DOUBLE tspTourLength;
  const Point &origin;
  const Point &destination;
  SegmentSite(const Point &_origin, const Point &_destination);
  static SegmentSite *random(const DOUBLE scale);
  virtual DOUBLE distance(const Distance *measure, const Point &p) const;
  virtual void boundDistance(const Distance *measure, const Point &centre,
                             DOUBLE halfwidth, Range &range) const;
  void setTspTourLength(DOUBLE val) { tspTourLength = val; }
  DOUBLE getTspTourLength() const { return tspTourLength; }
  Point getOrigin() const { return origin; }
  Point getDestination() const { return destination; }
};

/*********************************************************
 * Highway map headers                                    *
 *********************************************************/

struct ValueId {
  DOUBLE value;
  int id;
  ValueId(DOUBLE _value, int _id);
  bool operator<(const ValueId &vi) const;
};

class Canvas;

struct HighwaySection {
  Point p[2];
  Point invVector[2]; // 2x2 matrix row by row such that
                      // invVector * (p[1] - p[0]) = (1,0).
  DOUBLE speed;

  DOUBLE cotanOfEntry;
  // used with Euclidean distance

  DOUBLE invWidth;  // signed, i.e. 1 / travel.x();
  DOUBLE invHeight; // signed, i.e. 1 / travel.y();
  DOUBLE orientation;
  // used with Manhattan distance:
  // > 1: north-south: highway is always approached EastWest
  // [-1,1]: diagonal, direction of approach depends on destination
  // < -1: east-west: highway is always approached NorthSouth

  std::vector<ValueId> ramps;
  HighwaySection(const Point &p0, const Point &p1, DOUBLE _speed);
  void calculateInterchange(HighwaySection &other, DOUBLE &tSelf,
                            DOUBLE &tOther);
  Point rampLocation(const DOUBLE t) const;

  void drawOutline(Canvas &canvas, const RGB &colour) const;
  void drawFill(Canvas &canvas, const RGB &colour) const;
};

template <class E>
// E must provide:
// static DOUBLE crossCountryDistance(const Point& p, const Point& q);
// static void calculateRamps(const HighwaySection& hw, DOUBLE* t, const Point&
// q);
class HighwayMap {
private:
  std::vector<HighwaySection *> highways;
  // O1

  struct HighwayNode {
    Point location;
    std::vector<ValueId> neighbours;
    DOUBLE *distances;
    HighwayNode(const Point &_location, int nrSites);
    void connect(int nodeId, DOUBLE distance);
  };
  std::vector<HighwayNode> nodes;
  int nrPrimaryNodes;

  struct Cell {
    std::vector<int> nodes;
    std::vector<int> sections;
    std::vector<Site *> sites;
  };
  Cell ***cells; // usage: cells[groupId][gridRow][gridColumn]
  const int gridWidth;
  const DOUBLE cellsPerUnit;

  struct Probe {
    DOUBLE distance;
    int nodeId;
    int siteId;
    Probe(DOUBLE _distance, int _nodeId, int _siteId);
    bool operator<(const Probe &p) const;
  };

  // O2

  DOUBLE distance(const Point &p, HighwayNode &n, int siteId);
  DOUBLE distance(const Point &p, HighwaySection &hw, int siteId);

public:
  HighwayMap(int nrGroups, std::vector<Site *> &sites,
             std::vector<HighwaySection *> &_highways);
  DOUBLE distance(const Point &p, int siteId);

  // for debugging purposes:
  void showNodes(Canvas &canvas);
};

struct EuclideanHighwayEnvironment {
  static DOUBLE crossCountryDistance(const Point &p, const Point &q);
  static void calculateRamps(const HighwaySection &hw, DOUBLE *t,
                             const Point &q);
};

struct ManhattanHighwayEnvironment {
  static DOUBLE crossCountryDistance(const Point &p, const Point &q);
  static void calculateRamps(const HighwaySection &hw, DOUBLE *t,
                             const Point &q);
};

/*********************************************************
 * Distance headers                                       *
 *********************************************************/

class Distance {
public:
  virtual const std::string description() const;
  virtual SiteTypes supportedSites() const;
  bool supports(const SiteTypes s) const;

  // functions to compute distances
  virtual DOUBLE distanceToSite(const Point &p, const Site *s) const;
  virtual void boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                   const Site *s, Range &range) const;
  virtual DOUBLE distanceToPointSite(const Point &p, const PointSite *s) const;
  virtual void boundDistanceToPointSite(const Point &centre, DOUBLE halfwidth,
                                        const PointSite *s, Range &range) const;
  virtual DOUBLE
  distanceToSphericalPointSite(const Point &p,
                               const SphericalPointSite *s) const;
  virtual void boundDistanceToSphericalPointSite(const Point &centre,
                                                 DOUBLE halfwidth,
                                                 const SphericalPointSite *s,
                                                 Range &range) const;
  virtual DOUBLE
  distanceToHyperbolicPointSite(const Point &p,
                                const HyperbolicPointSite *s) const;
  virtual void boundDistanceToHyperbolicPointSite(const Point &centre,
                                                  DOUBLE halfwidth,
                                                  const HyperbolicPointSite *s,
                                                  Range &range) const;
  virtual DOUBLE distanceToDirectedPointSite(const Point &p,
                                             const DirectedPointSite *s) const;
  virtual void boundDistanceToDirectedPointSite(const Point &centre,
                                                DOUBLE halfwidth,
                                                const DirectedPointSite *s,
                                                Range &range) const;
  virtual DOUBLE distanceToBigSite(const Point &p, const BigSite *s) const;
  virtual void boundDistanceToBigSite(const Point &centre, DOUBLE halfwidth,
                                      const BigSite *s, Range &range) const;
  virtual DOUBLE distanceToRootedVectorSite(const Point &p,
                                            const RootedVectorSite *s) const;
  virtual void boundDistanceToRootedVectorSite(const Point &centre,
                                               DOUBLE halfwidth,
                                               const RootedVectorSite *s,
                                               Range &range) const;
  virtual DOUBLE distanceToPolylineSite(const Point &p,
                                        const PolylineSite *s) const;
  virtual void boundDistanceToPolylineSite(const Point &centre,
                                           DOUBLE halfwidth,
                                           const PolylineSite *s,
                                           Range &range) const;
  virtual DOUBLE distanceToSegmentSite(const Point &p,
                                       const SegmentSite *s) const;
  virtual void boundDistanceToSegmentSite(const Point &centre, DOUBLE halfwidth,
                                          const SegmentSite *s,
                                          Range &range) const;
};

class FieldDistance : public Distance
// this class is intended for variables that do not depend on the sites,
// but only on the point in the plane whose region we want to compute
{
public:
  virtual SiteTypes supportedSites() const;
};

class ConstantDistance : public FieldDistance {
  DOUBLE weight;

public:
  ConstantDistance(const DOUBLE _weight);
  virtual const std::string description() const;
  virtual DOUBLE distanceToSite(const Point &p, const Site *s) const;
  virtual void boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                   const Site *s, Range &range) const;
};

class FieldXDistance : public FieldDistance {
public:
  virtual const std::string description() const;
  virtual DOUBLE distanceToSite(const Point &p, const Site *s) const;
  virtual void boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                   const Site *s, Range &range) const;
};

class FieldYDistance : public FieldDistance {
public:
  virtual const std::string description() const;
  virtual DOUBLE distanceToSite(const Point &p, const Site *s) const;
  virtual void boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                   const Site *s, Range &range) const;
};

class FieldRDistance : public FieldDistance {
public:
  virtual const std::string description() const;
  virtual DOUBLE distanceToSite(const Point &p, const Site *s) const;
  virtual void boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                   const Site *s, Range &range) const;
};

class SiteDistance : public Distance
// this class is intended for variables that depend only on the sites,
// not on the point in the plane whose region we want to compute
{
public:
  virtual void boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                   const Site *s, Range &range) const;
};

class SiteXDistance : public SiteDistance {
public:
  virtual const std::string description() const;
  virtual SiteTypes supportedSites() const;
  virtual DOUBLE distanceToPointSite(const Point &p, const PointSite *s) const;
};

class SiteYDistance : public SiteDistance {
public:
  virtual const std::string description() const;
  virtual SiteTypes supportedSites() const;
  virtual DOUBLE distanceToPointSite(const Point &p, const PointSite *s) const;
};

class SiteRDistance : public SiteDistance {
public:
  virtual const std::string description() const;
  virtual SiteTypes supportedSites() const;
  virtual DOUBLE distanceToPointSite(const Point &p, const PointSite *s) const;
};

template <char S> class BinaryCompositionDistance : public Distance {
protected:
  const Distance *a;
  const Distance *b;

public:
  BinaryCompositionDistance(const Distance *_a, const Distance *_b);
  virtual const std::string description() const;
  virtual SiteTypes supportedSites() const;
};

class MinDistance : public BinaryCompositionDistance<'_'> {
public:
  MinDistance(Distance *_a, Distance *_b)
      : BinaryCompositionDistance(_a, _b) {};
  virtual DOUBLE distanceToSite(const Point &p, const Site *s) const;
  virtual void boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                   const Site *s, Range &range) const;
};

class MaxDistance : public BinaryCompositionDistance<'|'> {
public:
  MaxDistance(const Distance *_a, const Distance *_b)
      : BinaryCompositionDistance(_a, _b) {};
  virtual DOUBLE distanceToSite(const Point &p, const Site *s) const;
  virtual void boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                   const Site *s, Range &range) const;
};

class SumDistance : public BinaryCompositionDistance<'+'> {
public:
  SumDistance(const Distance *_a, const Distance *_b)
      : BinaryCompositionDistance(_a, _b) {};
  virtual DOUBLE distanceToSite(const Point &p, const Site *s) const;
  virtual void boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                   const Site *s, Range &range) const;
};

class DifferenceDistance : public BinaryCompositionDistance<'-'> {
public:
  DifferenceDistance(const Distance *_a, const Distance *_b)
      : BinaryCompositionDistance(_a, _b) {};
  virtual DOUBLE distanceToSite(const Point &p, const Site *s) const;
  virtual void boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                   const Site *s, Range &range) const;
};

class AbsoluteDifferenceDistance : public BinaryCompositionDistance<'~'> {
public:
  AbsoluteDifferenceDistance(const Distance *_a, const Distance *_b)
      : BinaryCompositionDistance(_a, _b) {};
  virtual DOUBLE distanceToSite(const Point &p, const Site *s) const;
  virtual void boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                   const Site *s, Range &range) const;
};

class ProductDistance : public BinaryCompositionDistance<'*'> {
public:
  ProductDistance(const Distance *_a, const Distance *_b)
      : BinaryCompositionDistance(_a, _b) {};
  virtual DOUBLE distanceToSite(const Point &p, const Site *s) const;
  virtual void boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                   const Site *s, Range &range) const;
};

class RatioDistance : public BinaryCompositionDistance<'/'> {
public:
  RatioDistance(const Distance *_a, const Distance *_b)
      : BinaryCompositionDistance(_a, _b) {};
  virtual DOUBLE distanceToSite(const Point &p, const Site *s) const;
  virtual void boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                   const Site *s, Range &range) const;
};

class PowerDistance : public BinaryCompositionDistance<'^'> {
public:
  PowerDistance(const Distance *_a, const Distance *_b)
      : BinaryCompositionDistance(_a, _b) {};
  virtual DOUBLE distanceToSite(const Point &p, const Site *s) const;
  virtual void boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                   const Site *s, Range &range) const;
};

class LessDistance : public BinaryCompositionDistance<'<'> {
public:
  LessDistance(const Distance *_a, const Distance *_b)
      : BinaryCompositionDistance(_a, _b) {};
  virtual DOUBLE distanceToSite(const Point &p, const Site *s) const;
  virtual void boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                   const Site *s, Range &range) const;
};

class ConditionalDistance : public BinaryCompositionDistance<'?'> {
public:
  ConditionalDistance(const Distance *_a, const Distance *_b)
      : BinaryCompositionDistance(_a, _b) {};
  virtual DOUBLE distanceToSite(const Point &p, const Site *s) const;
  virtual void boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                   const Site *s, Range &range) const;
};

class UnaryOperatorDistance : public Distance {
protected:
  const Distance *a;
  const std::string symbol;

public:
  UnaryOperatorDistance(const Distance *_a, const char *_symbol);
  virtual const std::string description() const;
  virtual SiteTypes supportedSites() const;
};

class LogDistance : public UnaryOperatorDistance {
public:
  LogDistance(const Distance *_a);
  virtual DOUBLE distanceToSite(const Point &p, const Site *s) const;
  virtual void boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                   const Site *s, Range &range) const;
};

class AbsDistance : public UnaryOperatorDistance {
public:
  AbsDistance(const Distance *_a);
  virtual DOUBLE distanceToSite(const Point &p, const Site *s) const;
  virtual void boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                   const Site *s, Range &range) const;
};

class SquareDistance : public UnaryOperatorDistance {
public:
  SquareDistance(const Distance *_a);
  virtual DOUBLE distanceToSite(const Point &p, const Site *s) const;
  virtual void boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                   const Site *s, Range &range) const;
};

class SquareRootDistance : public UnaryOperatorDistance {
public:
  SquareRootDistance(const Distance *_a);
  virtual DOUBLE distanceToSite(const Point &p, const Site *s) const;
  virtual void boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                   const Site *s, Range &range) const;
};

class ArccosDistance : public UnaryOperatorDistance {
public:
  ArccosDistance(const Distance *_a);
  virtual DOUBLE distanceToSite(const Point &p, const Site *s) const;
  virtual void boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                   const Site *s, Range &range) const;
};

class ArcoshDistance : public UnaryOperatorDistance {
public:
  ArcoshDistance(const Distance *_a);
  virtual DOUBLE distanceToSite(const Point &p, const Site *s) const;
  virtual void boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                   const Site *s, Range &range) const;
};

class PointDistance : public Distance {
public:
  virtual SiteTypes supportedSites() const;
};

class SegmentDistance : public Distance {
public:
  virtual SiteTypes supportedSites() const;
};

class PolylineDistance : public Distance {
public:
  virtual SiteTypes supportedSites() const;
};

class SetDistance : public Distance {
public:
  virtual SiteTypes supportedSites() const;
};

class DirectedPointDistance : public Distance {
public:
  virtual SiteTypes supportedSites() const;
};

class BigSiteDistance : public Distance {
public:
  virtual SiteTypes supportedSites() const;
};

class RootedVectorDistance : public Distance {
public:
  virtual SiteTypes supportedSites() const;
};

class TranslatedDistance : public UnaryOperatorDistance {
public:
  TranslatedDistance(const Distance *_a);
  virtual SiteTypes supportedSites() const;
  virtual DOUBLE distanceToPointSite(const Point &p, const PointSite *s) const;
  virtual void boundDistanceToPointSite(const Point &centre, DOUBLE halfwidth,
                                        const PointSite *s, Range &range) const;
};

class OrientedDistance : public UnaryOperatorDistance {
public:
  OrientedDistance(const Distance *_a);
  virtual SiteTypes supportedSites() const;
  virtual DOUBLE distanceToDirectedPointSite(const Point &p,
                                             const DirectedPointSite *s) const;
  virtual void boundDistanceToDirectedPointSite(const Point &centre,
                                                DOUBLE halfwidth,
                                                const DirectedPointSite *s,
                                                Range &range) const;
  virtual DOUBLE distanceToRootedVectorSite(const Point &p,
                                            const RootedVectorSite *s) const;
  virtual void boundDistanceToRootedVectorSite(const Point &centre,
                                               DOUBLE halfwidth,
                                               const RootedVectorSite *s,
                                               Range &range) const;
};

class EuclideanDistance : public SetDistance {
public:
  virtual const std::string description() const;
  virtual DOUBLE distanceToPointSite(const Point &p, const PointSite *s) const;
  virtual void boundDistanceToPointSite(const Point &centre, DOUBLE halfwidth,
                                        const PointSite *s, Range &range) const;
  virtual DOUBLE distanceToSegmentSite(const Point &p,
                                       const SegmentSite *s) const;
  virtual void boundDistanceToSegmentSite(const Point &centre, DOUBLE halfwidth,
                                          const SegmentSite *s,
                                          Range &range) const;
};

class SquaredEuclideanDistance : public SetDistance {
public:
  virtual const std::string description() const;
  virtual DOUBLE distanceToPointSite(const Point &p, const PointSite *s) const;
  virtual void boundDistanceToPointSite(const Point &centre, DOUBLE halfwidth,
                                        const PointSite *s, Range &range) const;
  virtual DOUBLE distanceToSegmentSite(const Point &p,
                                       const SegmentSite *s) const;
  virtual void boundDistanceToSegmentSite(const Point &centre, DOUBLE halfwidth,
                                          const SegmentSite *s,
                                          Range &range) const;
};

class EuclideanHighwayDistance : public PointDistance {
public:
  static bool mapNeeded;
  static HighwayMap<EuclideanHighwayEnvironment> *map;
  EuclideanHighwayDistance();
  virtual const std::string description() const;
  virtual DOUBLE distanceToPointSite(const Point &p, const PointSite *s) const;
  virtual void boundDistanceToPointSite(const Point &centre, DOUBLE halfwidth,
                                        const PointSite *s, Range &range) const;
};

class ManhattanDistance : public SetDistance {
public:
  virtual const std::string description() const;
  virtual DOUBLE distanceToPointSite(const Point &p, const PointSite *s) const;
  virtual void boundDistanceToPointSite(const Point &centre, DOUBLE halfwidth,
                                        const PointSite *s, Range &range) const;
  virtual DOUBLE distanceToSegmentSite(const Point &p,
                                       const SegmentSite *s) const;
  virtual void boundDistanceToSegmentSite(const Point &centre, DOUBLE halfwidth,
                                          const SegmentSite *s,
                                          Range &range) const;
};

class ManhattanHighwayDistance : public PointDistance {
public:
  static bool mapNeeded;
  static HighwayMap<ManhattanHighwayEnvironment> *map;
  ManhattanHighwayDistance();
  virtual const std::string description() const;
  virtual DOUBLE distanceToPointSite(const Point &p, const PointSite *s) const;
  virtual void boundDistanceToPointSite(const Point &centre, DOUBLE halfwidth,
                                        const PointSite *s, Range &range) const;
};

class cTriangularGridDistance : public FieldDistance {
public:
  virtual const std::string description() const;
  virtual DOUBLE distanceToSite(const Point &p, const Site *s) const;
  virtual void boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                   const Site *s, Range &range) const;
};

template <class P>
// P should be a class with members:
//   static bool projectOntoSphere(const Point& inPlane, SphericalPoint&
//   onSphere),
// which returns false if inPlane lies outside the image of the sphere,
// otherwise returns true and puts the 3D coordinates of the point on the
// sphere in onSphere;
//   static void projectOntoSphere(
//     const Point& inPlane, const Point& halfWidth,
//     SphericalPoint& centre, DOUBLE& radius
//   ),
// which computes the centre and the radius of a spherical circle that includes
// the rectangle on the map around inPlane with sides 2*halfWidth;
//   static const SphericalPoint centre,
// which gives the point in the centre of the map;
//   static const std::string description(),
// which gives the name of the projection.
class SphericalDistance : public PointDistance {
private:
  DOUBLE aspectRatio;
  DOUBLE radius;
  DOUBLE cosRadius;

public:
  SphericalDistance(const DOUBLE _aspectRatio, const DOUBLE _radius);
  virtual const std::string description() const;
  virtual SiteTypes supportedSites() const;
  virtual DOUBLE distanceToPointSite(const Point &p, const PointSite *s) const;
  virtual void boundDistanceToPointSite(const Point &centre, DOUBLE halfwidth,
                                        const PointSite *s, Range &range) const;
  virtual DOUBLE
  distanceToSphericalPointSite(const Point &p,
                               const SphericalPointSite *s) const;
  virtual void boundDistanceToSphericalPointSite(const Point &centre,
                                                 DOUBLE halfwidth,
                                                 const SphericalPointSite *s,
                                                 Range &range) const;
};

template <class Q>
struct AzimuthalProjection
// Q should be a class with members:
//   static const std::string description,
// which is the keyword for the projection;
//   static DOUBLE height(const DOUBLE rr),
// which returns the height (sine of the latitude) as a function
// of the squared distance from the centre of the map;
//   static void getHeightAndScale(const DOUBLE rr, DOUBLE& _height, DOUBLE&
//   _scale),
// which calculates the height and the scale factor for the horizontal distance
// as a function of the square distance from the centre of the map
{
  static const std::string description();
  static const SphericalPoint centre;
  static bool projectOntoSphere(const Point &inPlane, SphericalPoint &onSphere);
  static void projectOntoSphere(const Point &inPlane, const Point &halfWidth,
                                SphericalPoint &centre, DOUBLE &radius);
};

struct AzimuthalEqualAreaProjection {
  static const std::string description;
  static DOUBLE height(const DOUBLE rr);
  static void getHeightAndScale(const DOUBLE rr, DOUBLE &_height,
                                DOUBLE &_scale);
};

struct StereographicProjection {
  static const std::string description;
  static DOUBLE height(const DOUBLE rr);
  static void getHeightAndScale(const DOUBLE rr, DOUBLE &_height,
                                DOUBLE &_scale);
};

struct EquidistantProjection {
  static const std::string description;
  static DOUBLE height(const DOUBLE rr);
  static void getHeightAndScale(const DOUBLE rr, DOUBLE &_height,
                                DOUBLE &_scale);
};

struct GnomonicProjection {
  static const std::string description;
  static DOUBLE height(const DOUBLE rr);
  static void getHeightAndScale(const DOUBLE rr, DOUBLE &_height,
                                DOUBLE &_scale);
};

struct OrthographicProjection {
  static const std::string description;
  static DOUBLE height(const DOUBLE rr);
  static void getHeightAndScale(const DOUBLE rr, DOUBLE &_height,
                                DOUBLE &_scale);
};

template <class Q>
struct CylindricalProjection
// Q should be a class with members:
//   static const std::string description,
// which is the keyword for the projection;
//   static DOUBLE height(const DOUBLE y),
// which returns the height (sine of the latitude) as a function
// of the distance from the equator on the map;
{
  static const std::string description();
  static const SphericalPoint centre;
  static bool projectOntoSphere(const Point &inPlane, SphericalPoint &onSphere);
  static void projectOntoSphere(const Point &inPlane, const Point &halfWidth,
                                SphericalPoint &centre, DOUBLE &radius);
};

struct CentralCylindricalProjection {
  static const std::string description;
  static DOUBLE height(const DOUBLE y);
};

struct CylindricalEqualAreaProjection
// latitude of no distortion = acos(sqrt(aspect ratio / pi)), i.e.
// aspect ratio = pi cos^2 (latitude of no distortion)
{
  static const std::string description;
  static DOUBLE height(const DOUBLE y);
};

struct EquirectangularProjection {
  static const std::string description;
  static DOUBLE height(const DOUBLE y);
};

struct MercatorProjection {
  static const std::string description;
  static DOUBLE height(const DOUBLE y);
};

struct MollweideProjection {
  static const std::string description();
  static const SphericalPoint centre;
  static bool projectOntoSphere(const Point &inPlane, SphericalPoint &onSphere);
  static void projectOntoSphere(const Point &inPlane, const Point &halfWidth,
                                SphericalPoint &centre, DOUBLE &radius);
};

struct SinusoidalProjection {
  static const std::string description();
  static const SphericalPoint centre;
  static bool projectOntoSphere(const Point &inPlane, SphericalPoint &onSphere);
  static void projectOntoSphere(const Point &inPlane, const Point &halfWidth,
                                SphericalPoint &centre, DOUBLE &radius);
};

struct HammerProjection {
  static const std::string description();
  static const SphericalPoint centre;
  static bool projectOntoSphere(const Point &inPlane, SphericalPoint &onSphere);
  static void projectOntoSphere(const Point &inPlane, const Point &halfWidth,
                                SphericalPoint &centre, DOUBLE &radius);
};

struct AitoffProjection {
  static const std::string description();
  static const SphericalPoint centre;
  static bool projectOntoSphere(const Point &inPlane, SphericalPoint &onSphere);
  static void projectOntoSphere(const Point &inPlane, const Point &halfWidth,
                                SphericalPoint &centre, DOUBLE &radius);
};

template <int N> struct PetalProjection {
  static const std::string description();
  static bool projectOntoSphere(const Point &inPlane, SphericalPoint &onSphere);
  static void projectOntoSphere(const Point &inPlane, const Point &halfWidth,
                                SphericalPoint &centre, DOUBLE &radius);
};

struct WernerProjection {
  static const std::string description();
  static const SphericalPoint centre;
  static bool projectOntoSphere(const Point &inPlane, SphericalPoint &onSphere);
  static void projectOntoSphere(const Point &inPlane, const Point &halfWidth,
                                SphericalPoint &centre, DOUBLE &radius);
};

class cLMinDistance : public FieldDistance {
public:
  virtual const std::string description() const;
  virtual DOUBLE distanceToSite(const Point &p, const Site *s) const;
  virtual void boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                   const Site *s, Range &range) const;
};

class cVolumeDistance : public FieldDistance {
public:
  virtual const std::string description() const;
  virtual DOUBLE distanceToSite(const Point &p, const Site *s) const;
  virtual void boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                   const Site *s, Range &range) const;
};

class cLMaxDistance : public FieldDistance {
public:
  virtual const std::string description() const;
  virtual DOUBLE distanceToSite(const Point &p, const Site *s) const;
  virtual void boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                   const Site *s, Range &range) const;
};

class cLDistance : public FieldDistance {
private:
  DOUBLE n;
  DOUBLE invn;

public:
  cLDistance(const DOUBLE _n);
  virtual const std::string description() const;
  virtual DOUBLE distanceToSite(const Point &p, const Site *s) const;
  virtual void boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                   const Site *s, Range &range) const;
};

template <class P>
// P should be a class with members:
//   static bool projectOntoGansPlane(const Point& preimage, GansPoint& image),
// which returns false if the preimage lies outside the preimage of the Gans
// plane, otherwise returns true and computes the image on the Gans plane;
//   static void projectOntoGansPlane(
//     const Point& inPlane, const Point& halfWidth,
//     GansPoint& centre, DOUBLE& radius
//   ),
// which computes the centre and the radius of a hyperbolic circle that includes
// the rectangle on the map around inPlane with sides 2*halfWidth;
//   static const std::string description,
// which gives the name of the projection
class HyperbolicDistance : public PointDistance {
public:
  HyperbolicDistance();
  virtual const std::string description() const;
  virtual SiteTypes supportedSites() const;
  virtual DOUBLE distanceToPointSite(const Point &p, const PointSite *s) const;
  virtual void boundDistanceToPointSite(const Point &centre, DOUBLE halfwidth,
                                        const PointSite *s, Range &range) const;
  virtual DOUBLE
  distanceToHyperbolicPointSite(const Point &p,
                                const HyperbolicPointSite *s) const;
  virtual void boundDistanceToHyperbolicPointSite(const Point &centre,
                                                  DOUBLE halfwidth,
                                                  const HyperbolicPointSite *s,
                                                  Range &range) const;
};

struct GansProjection {
  static const std::string description;
  static bool projectOntoGansPlane(const Point &preimage, GansPoint &image);
  static void projectOntoGansPlane(const Point &inPlane, const Point &halfWidth,
                                   GansPoint &centre, DOUBLE &radius);
};

struct InvertedGansProjection {
  static const std::string description;
  static bool projectOntoGansPlane(const Point &preimage, GansPoint &image);
  static void projectOntoGansPlane(const Point &inPlane, const Point &halfWidth,
                                   GansPoint &centre, DOUBLE &radius);
};

struct PoincareDiskProjection {
  static const std::string description;
  static bool projectOntoGansPlane(const Point &preimage, GansPoint &image);
  static void projectOntoGansPlane(const Point &inPlane, const Point &halfWidth,
                                   GansPoint &centre, DOUBLE &radius);
};

struct PoincareHalfplaneProjection {
  static const std::string description;
  static bool projectOntoGansPlane(const Point &preimage, GansPoint &image);
  static void projectOntoGansPlane(const Point &inPlane, const Point &halfWidth,
                                   GansPoint &centre, DOUBLE &radius);
};

struct KleinProjection {
  static const std::string description;
  static bool projectOntoGansPlane(const Point &preimage, GansPoint &image);
  static void projectOntoGansPlane(const Point &inPlane, const Point &halfWidth,
                                   GansPoint &centre, DOUBLE &radius);
};

struct HyperbolicEqualAreaProjection {
  static const std::string description;
  static bool projectOntoGansPlane(const Point &preimage, GansPoint &image);
  static void projectOntoGansPlane(const Point &inPlane, const Point &halfWidth,
                                   GansPoint &centre, DOUBLE &radius);
};

struct HyperbolicEqualDistanceProjection {
  static const std::string description;
  static bool projectOntoGansPlane(const Point &preimage, GansPoint &image);
  static void projectOntoGansPlane(const Point &inPlane, const Point &halfWidth,
                                   GansPoint &centre, DOUBLE &radius);
};

class InvertedEuclideanDistance : public PointDistance {
public:
  virtual const std::string description() const;
  virtual DOUBLE distanceToPointSite(const Point &p, const PointSite *s) const;
  virtual void boundDistanceToPointSite(const Point &centre, DOUBLE halfwidth,
                                        const PointSite *s, Range &range) const;
};

class KarlsruheDistance : public PointDistance {
public:
  virtual const std::string description() const;
  virtual DOUBLE distanceToPointSite(const Point &p, const PointSite *s) const;
  virtual void boundDistanceToPointSite(const Point &centre, DOUBLE halfwidth,
                                        const PointSite *s, Range &range) const;
};

class AzimuthDistance : public PointDistance {
public:
  virtual const std::string description() const;
  virtual DOUBLE distanceToPointSite(const Point &p, const PointSite *s) const;
  virtual void boundDistanceToPointSite(const Point &centre, DOUBLE halfwidth,
                                        const PointSite *s, Range &range) const;
};

class AngleDistance : public PolylineDistance {
public:
  virtual const std::string description() const;
  virtual DOUBLE distanceToPolylineSite(const Point &p,
                                        const PolylineSite *s) const;
  virtual void boundDistanceToPolylineSite(const Point &centre,
                                           DOUBLE halfwidth,
                                           const PolylineSite *s,
                                           Range &range) const;
};

class DetourDistance : public SegmentDistance {
public:
  virtual const std::string description() const;
  virtual DOUBLE distanceToSegmentSite(const Point &p,
                                       const SegmentSite *s) const;
  virtual void boundDistanceToSegmentSite(const Point &centre, DOUBLE halfwidth,
                                          const SegmentSite *s,
                                          Range &range) const;
};

class DilationDistance : public SegmentDistance {
public:
  virtual const std::string description() const;
  virtual DOUBLE distanceToSegmentSite(const Point &p,
                                       const SegmentSite *s) const;
  virtual void boundDistanceToSegmentSite(const Point &centre, DOUBLE halfwidth,
                                          const SegmentSite *s,
                                          Range &range) const;
};

class cSecantDistance : public FieldDistance {
public:
  virtual const std::string description() const;
  virtual DOUBLE distanceToSite(const Point &p, const Site *s) const;
  virtual void boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                   const Site *s, Range &range) const;
};

class cCatchDistance : public FieldDistance {
public:
  virtual const std::string description() const;
  virtual DOUBLE distanceToSite(const Point &p, const Site *s) const;
  virtual void boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                   const Site *s, Range &range) const;
};

class cMixedCatchDistance : public BigSiteDistance {
private:
  DOUBLE speed;

public:
  cMixedCatchDistance(DOUBLE _speed);
  virtual const std::string description() const;
  virtual DOUBLE distanceToBigSite(const Point &p, const BigSite *s) const;
  virtual void boundDistanceToBigSite(const Point &centre, DOUBLE halfwidth,
                                      const BigSite *s, Range &range) const;
};

class cPushDistance : public BigSiteDistance {
private:
  DOUBLE acceleration;

public:
  cPushDistance(DOUBLE _acceleration);
  virtual const std::string description() const;
  virtual DOUBLE distanceToBigSite(const Point &p, const BigSite *s) const;
};

class cTurnDistance : public FieldDistance {
public:
  virtual const std::string description() const;
  virtual DOUBLE distanceToSite(const Point &p, const Site *s) const;
  virtual void boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                   const Site *s, Range &range) const;
};

class cLeftTurnDistance : public FieldDistance {
public:
  virtual const std::string description() const;
  virtual DOUBLE distanceToSite(const Point &p, const Site *s) const;
  virtual void boundDistanceToSite(const Point &centre, DOUBLE halfwidth,
                                   const Site *s, Range &range) const;
};

class cDubinsDistance : public BigSiteDistance {
public:
  virtual const std::string description() const;
  virtual DOUBLE distanceToBigSite(const Point &p, const BigSite *s) const;
  virtual void boundDistanceToBigSite(const Point &centre, DOUBLE halfwidth,
                                      const BigSite *s, Range &range) const;
};

Distance *getDistanceObject(std::string name);

/*********************************************************
 * Diagram headers                                        *
 *********************************************************/

struct Element {
  short group;
  DOUBLE distance;
  DOUBLE shade;
  short contour;
  Element();
  static const short softEdge;
  static const short minorContour;
  static const short majorContour;
  static const short bisector;
};

void putContour(const short level, Element &e1);
void putContour(const short level, Element &e1, Element &e2);

/*********************************************************
 * Owner header                                           *
 *********************************************************/

struct Owner // (Region ID)
{
  int primary;   // primary site number;   nrGroups if none
  int secondary; // secondary site number; nrGroups if none
  Owner();
  const bool operator==(const Owner &o) const;
  const bool operator!=(const Owner &o) const;
};

template <class T> bool same(const T &a, const T &b, const T &c, const T &d);

/*********************************************************
 * Sample header                                           *
 *********************************************************/

struct Sample {
  Owner owner;
  DOUBLE *distance;
  int *layer;
  DOUBLE sunlight;
};

/**********************************************************************/

#define VOROSKETCH_HEADERS
// optional: cut file in .h and .cpp part here
// :x-----------------------------------------
} // namespace vorosketch
