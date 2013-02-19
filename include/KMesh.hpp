#ifndef ___GFTOOLS_KMESH_HPP___
#define ___GFTOOLS_KMESH_HPP___

#include "Grid.hpp"
#include <numeric>

namespace GFTools { 

class KMesh : public Grid<RealType, KMesh>
{
public:
    int _points;
    KMesh(size_t n_points);
    KMesh(const KMesh& rhs);
    KMesh(KMesh &&rhs);
    KMesh(){};
    KMesh& operator=(KMesh &&rhs){_points = rhs._points; _vals.swap(rhs._vals); return (*this);};
    KMesh& operator=(const KMesh &rhs){_points = rhs._points; _vals = rhs._vals; return (*this);};
    std::tuple <bool, size_t, RealType> find (RealType in) const ;
    template <class Obj> auto integrate(const Obj &in) const ->decltype(in(_vals[0]));
    template <class Obj, typename ...OtherArgTypes> auto integrate(const Obj &in, OtherArgTypes... Args) const -> decltype(in(_vals[0],Args...));
    //template <class Obj> auto gridIntegrate(std::vector<Obj> &in) const -> Obj;
    template <class Obj> auto getValue(Obj &in, RealType x) const ->decltype(in[0]);
    template <class Obj> auto getValue(Obj &in, point x) const ->decltype(in[0]);
    template <class ArgType> point shift(point in, ArgType shift_arg) const;
    template <class ArgType> RealType shift(RealType in, ArgType shift_arg) const;
};

struct KMeshPatch : public KMesh 
{
    std::map<size_t,size_t> _map_vals;
public:
    const KMesh& _parent;
    size_t _npoints;
    using KMesh::_vals;
    KMeshPatch(const KMesh& parent, std::vector<size_t> indices);
    KMeshPatch(const KMesh& parent);
    template <class Obj> auto getValue(Obj &in, RealType x) const ->decltype(in[0]);
    template <class Obj> auto getValue(Obj &in, KMesh::point x) const ->decltype(in[0]);
    size_t getIndex(KMesh::point x) const;
};

//
// KMesh
//

inline KMesh::KMesh(size_t n_points):
Grid<RealType,KMesh>(0,n_points,[n_points](size_t in){return 2.0*PI/n_points*in;}),
_points(n_points)
{
}

inline KMesh::KMesh(const KMesh& rhs):Grid(rhs._vals),_points(rhs._points)
{
}

inline KMesh::KMesh(KMesh &&rhs):Grid(rhs._vals),_points(rhs._points)
{
}

inline std::tuple <bool, size_t, RealType> KMesh::find (RealType in) const
{
    assert(in>=0 && in < 2.0*PI);
    int n = std::lround(in/2.0/PI*_points);
    if (n<0) { ERROR("KMesh point is out of bounds, " << in << "<" << 0); return std::make_tuple(0,0,0); };
    if (n==_points) n=0; 
    if (n>_points) { ERROR("KMesh point is out of bounds, " << in << "> 2*PI"); return std::make_tuple(0,_points,0); };
    RealType weight=in/2.0/PI*_points-RealType(n);
    return std::make_tuple (1,n,weight);
}

template <class Obj>
inline auto KMesh::getValue(Obj &in, RealType x) const ->decltype(in[0]) 
{
    const auto find_result=this->find(x);
    if (!std::get<0>(find_result)) throw (exWrongIndex()); 
    return in[std::get<1>(find_result)];
}


template <class Obj>
inline auto KMesh::getValue(Obj &in, KMesh::point x) const ->decltype(in[0]) 
{
    if (x._index < _vals.size() && x == _vals[x._index])
    return in[x._index];
    else { 
        #ifndef NDEBUG
        ERROR ("Point not found"); 
        #endif
        return this->getValue(in, RealType(x)); 
         };
}

template <class Obj> 
inline auto KMesh::integrate(const Obj &in) const -> decltype(in(_vals[0]))
{
    decltype(in(_vals[0])) R = in(RealType(_vals[0]));
    R=std::accumulate(_vals.begin()+1, _vals.end(), R,[&](decltype(in(_vals[0]))& y,const decltype(_vals[0]) &x) {return y+in(x);}); 
    return R/_points;
}

template <class Obj, typename ...OtherArgTypes> 
inline auto KMesh::integrate(const Obj &in, OtherArgTypes... Args) const -> decltype(in(_vals[0],Args...))
{
    decltype(in(_vals[0],Args...)) R = in(_vals[0],Args...);
    R=std::accumulate(_vals.begin()+1, _vals.end(), R,[&](decltype(in(_vals[0]))& y,const decltype(_vals[0]) &x) {return y+in(x,Args...);}); 
    return R/_points;
}

template <class ArgType>
inline RealType KMesh::shift(RealType in, ArgType shift_arg) const
{
    assert (in>=0 && in < 2.0*PI);
    RealType out;
    out = in + RealType(shift_arg); 
    out-= std::floor(out/(2.0*PI))*2.0*PI;
    return out;
}


template <class ArgType>
inline typename KMesh::point KMesh::shift(point in, ArgType shift_arg) const
{
    point out;
    out._val = this->shift(in._val, shift_arg);
    auto find_result = this->find(out._val);
    if (!std::get<0>(find_result)) throw (exWrongIndex());
    out._index = std::get<1>(find_result);
    return (*this)[out._index];
}

//
// KMeshPatch
//


inline KMeshPatch::KMeshPatch(const KMesh& parent, std::vector<size_t> indices):
    _parent(parent),
    _npoints(indices.size())
{
    _vals.resize(_npoints); 
    for (size_t i=0; i<_npoints; ++i) {
        _vals[i]=_parent[indices[i]]; 
        _map_vals[size_t(_vals[i])] = i;
        }
}

inline KMeshPatch::KMeshPatch(const KMesh& parent):
    _parent(parent),
    _npoints(parent.getSize())
{
    _vals = parent.getPoints();
     for (size_t i=0; i<_npoints; ++i) {
        _map_vals[size_t(_vals[i])] = i;
        }
}

template <class Obj> 
inline auto KMeshPatch::getValue(Obj &in, RealType x) const ->decltype(in[0])
{
    const auto find_result=_parent.find(x);
    if (!std::get<0>(find_result)) throw (exWrongIndex()); 
    return getValue(in, KMesh::point(std::get<1>(find_result), x));
}

template <class Obj> 
inline auto KMeshPatch::getValue(Obj &in, KMesh::point x) const ->decltype(in[0])
{
    return in[getIndex(x)];
}

inline size_t KMeshPatch::getIndex(KMesh::point x) const
{
    auto f1 = _map_vals.find(size_t(x));
    if (f1!=_map_vals.end()) { return f1->second; }
    else throw (exWrongIndex());
}

} // end of namespace GFTools
#endif // endif :: ifndef ___GFTOOLS_KMESH_HPP___