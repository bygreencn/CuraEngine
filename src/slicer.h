/** Copyright (C) 2013 David Braam - Released under terms of the AGPLv3 License */
#ifndef SLICER_H
#define SLICER_H

#include "mesh.h"
#include "utils/polygon.h"
/*
    The Slicer creates layers of polygons from an optimized 3D model.
    The result of the Slicer is a list of polygons without any order or structure.
*/
namespace cura {

class SlicerSegment
{
public:
    Point start, end;
    int faceIndex;
    bool addedToPolygon;
};

class ClosePolygonResult
{   //The result of trying to find a point on a closed polygon line. This gives back the point index, the polygon index, and the point of the connection.
    //The line on which the point lays is between pointIdx-1 and pointIdx
public:
    Point intersectionPoint;
    int polygonIdx;
    unsigned int pointIdx;
};
class GapCloserResult
{
public:
    int64_t len;
    int polygonIdx;
    unsigned int pointIdxA;
    unsigned int pointIdxB;
    bool AtoB;
};

class SlicerLayer
{
public:
    std::vector<SlicerSegment> segmentList;
    std::unordered_map<int, int> face_idx_to_segment_index; // topology
    
    int z;
    Polygons polygonList;
    Polygons openPolylines;
    
    void makePolygons(Mesh* mesh, bool keepNoneClosed, bool extensiveStitching);

private:
    GapCloserResult findPolygonGapCloser(Point ip0, Point ip1)
    {
        GapCloserResult ret;
        ClosePolygonResult c1 = findPolygonPointClosestTo(ip0);
        ClosePolygonResult c2 = findPolygonPointClosestTo(ip1);
        if (c1.polygonIdx < 0 || c1.polygonIdx != c2.polygonIdx)
        {
            ret.len = -1;
            return ret;
        }
        ret.polygonIdx = c1.polygonIdx;
        ret.pointIdxA = c1.pointIdx;
        ret.pointIdxB = c2.pointIdx;
        ret.AtoB = true;
        
        if (ret.pointIdxA == ret.pointIdxB)
        {
            //Connection points are on the same line segment.
            ret.len = vSize(ip0 - ip1);
        }else{
            //Find out if we have should go from A to B or the other way around.
            Point p0 = polygonList[ret.polygonIdx][ret.pointIdxA];
            int64_t lenA = vSize(p0 - ip0);
            for(unsigned int i = ret.pointIdxA; i != ret.pointIdxB; i = (i + 1) % polygonList[ret.polygonIdx].size())
            {
                Point p1 = polygonList[ret.polygonIdx][i];
                lenA += vSize(p0 - p1);
                p0 = p1;
            }
            lenA += vSize(p0 - ip1);

            p0 = polygonList[ret.polygonIdx][ret.pointIdxB];
            int64_t lenB = vSize(p0 - ip1);
            for(unsigned int i = ret.pointIdxB; i != ret.pointIdxA; i = (i + 1) % polygonList[ret.polygonIdx].size())
            {
                Point p1 = polygonList[ret.polygonIdx][i];
                lenB += vSize(p0 - p1);
                p0 = p1;
            }
            lenB += vSize(p0 - ip0);
            
            if (lenA < lenB)
            {
                ret.AtoB = true;
                ret.len = lenA;
            }else{
                ret.AtoB = false;
                ret.len = lenB;
            }
        }
        return ret;
    }

    ClosePolygonResult findPolygonPointClosestTo(Point input)
    {
        ClosePolygonResult ret;
        for(unsigned int n=0; n<polygonList.size(); n++)
        {
            Point p0 = polygonList[n][polygonList[n].size()-1];
            for(unsigned int i=0; i<polygonList[n].size(); i++)
            {
                Point p1 = polygonList[n][i];
                
                //Q = A + Normal( B - A ) * ((( B - A ) dot ( P - A )) / VSize( A - B ));
                Point pDiff = p1 - p0;
                int64_t lineLength = vSize(pDiff);
                if (lineLength > 1)
                {
                    int64_t distOnLine = dot(pDiff, input - p0) / lineLength;
                    if (distOnLine >= 0 && distOnLine <= lineLength)
                    {
                        Point q = p0 + pDiff * distOnLine / lineLength;
                        if (shorterThen(q - input, 100))
                        {
                            ret.intersectionPoint = q;
                            ret.polygonIdx = n;
                            ret.pointIdx = i;
                            return ret;
                        }
                    }
                }
                p0 = p1;
            }
        }
        ret.polygonIdx = -1;
        return ret;
    }
};

class Slicer
{
public:
    std::vector<SlicerLayer> layers;
    
    Slicer(Mesh* mesh, int initial, int thickness, int layer_count, bool keepNoneClosed, bool extensiveStitching);

    void dumpSegmentsToHTML(const char* filename);

protected:
    Mesh* mesh;
    int layer_height_0;
    int layer_height;
    
    /*!
     * Create a SlicerSegment along the lines going through p0p1 (Start) and p0p2 (End)
     * 
     * \warning \p p0 may not have the same z as either \p p1 or \p p2
     * 
     * \param p The face vertice locations in the order the vertices are given in the face
     */
    SlicerSegment project2D(Point3 p[3], unsigned int idx_shared, unsigned int idx_first, unsigned int idx_second, int32_t z) const;
};

}//namespace cura

#endif//SLICER_H
