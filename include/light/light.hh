/*  Copyright (C) 1996-1997  Id Software, Inc.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

    See file, 'COPYING', for details.
*/

#ifndef __LIGHT_LIGHT_H__
#define __LIGHT_LIGHT_H__

#include <common/cmdlib.h>
#include <common/mathlib.h>
#include <common/bspfile.h>
#include <common/log.h>
#include <common/threads.h>
#include <common/polylib.h>

#include <light/litfile.hh>
#include <light/trace.hh>

#include <vector>
#include <map>
#include <string>
#include <cassert>
#include <limits>
#include <sstream>

#define ON_EPSILON    0.1
#define ANGLE_EPSILON 0.001

typedef struct {
    vec3_t color;
    vec3_t direction;
} lightsample_t;

static inline float LightSample_Brightness(const vec3_t color) {
    return ((color[0] + color[1] + color[2]) / 3.0);
}

class sun_t {
public:
    vec3_t sunvec;
    vec_t sunlight;
    vec3_t sunlight_color;
    struct sun_s *next;
    qboolean dirt;
    float anglescale;
};

/* for vanilla this would be 18. some engines allow higher limits though, which will be needed if we're scaling lightmap resolution. */
/*with extra sampling, lit+lux etc, we need at least 46mb stack space per thread. yes, that's a lot. on the plus side, it doesn't affect bsp complexity (actually, can simplify it a little)*/
#define MAXDIMENSION (255+1)

typedef struct {
    vec3_t data[3];     /* permuted 3x3 matrix */
    int row[3];         /* row permutations */
    int col[3];         /* column permutations */
} pmatrix3_t;
    
typedef struct {
    pmatrix3_t transform;
    const texinfo_t *texinfo;
    vec_t planedist;
} texorg_t;

class modelinfo_t;
class globalconfig_t;

/*Warning: this stuff needs explicit initialisation*/
typedef struct {
    const globalconfig_t *cfg;
    const modelinfo_t *modelinfo;
    const bsp2_t *bsp;
    const bsp2_dface_t *face;
    /* these take precedence the values in modelinfo */
    vec_t minlight;
    vec3_t minlight_color;
    qboolean nodirt;
    
    plane_t plane;
    vec3_t snormal;
    vec3_t tnormal;
    
    /* 16 in vanilla. engines will hate you if this is not power-of-two-and-at-least-one */
    float lightmapscale;
    qboolean curved; /*normals are interpolated for smooth lighting*/
    
    int texmins[2];
    int texsize[2];
    vec_t exactmid[2];
    vec3_t midpoint;
    
    int numpoints;
    vec3_t *points; // malloc'ed array of numpoints
    vec3_t *normals; // malloc'ed array of numpoints
    bool *occluded; // malloc'ed array of numpoints
    
    /*
     raw ambient occlusion amount per sample point, 0-1, where 1 is
     fully occluded. dirtgain/dirtscale are not applied yet
     */
    vec_t *occlusion; // malloc'ed array of numpoints
    
    /* for sphere culling */
    vec3_t origin;
    vec_t radius;
    /* for AABB culling */
    vec3_t mins, maxs;

    // for radiosity
    vec3_t radiosity;
    vec3_t texturecolor;
    
    /* stuff used by CalcPoint */
    vec_t starts, startt, st_step;
    texorg_t texorg;
    int width, height;
    
    /* for lit water. receive light from either front or back. */
    bool twosided;
    
    // ray batch stuff
    raystream_t *stream;
} lightsurf_t;

typedef struct {
    int style;
    lightsample_t *samples; // malloc'ed array of numpoints   //FIXME: this is stupid, we shouldn't need to allocate extra data here for -extra4
} lightmap_t;

struct ltface_ctx
{
    const bsp2_t *bsp;
    lightsurf_t *lightsurf;
    const globalconfig_t *cfg;
    lightmap_t lightmaps[MAXLIGHTMAPS + 1];
};

extern struct ltface_ctx *ltface_ctxs;

/* debug */

typedef enum {
    debugmode_none = 0,
    debugmode_phong,
    debugmode_dirt,
    debugmode_bounce
} debugmode_t;

extern debugmode_t debugmode;
    
/* bounce lights */

typedef struct {
    vec3_t pos;
    vec3_t color;
    vec3_t surfnormal;
    vec_t area;
    const bsp2_dleaf_t *leaf;
    
    /* estimated visible AABB culling */
    vec3_t mins;
    vec3_t maxs;
} bouncelight_t;

extern byte thepalette[768];
    
/* tracelist is a std::vector of pointers to modelinfo_t to use for LOS tests */
extern std::vector<const modelinfo_t *> tracelist;
extern std::vector<const modelinfo_t *> selfshadowlist;

void vec_from_mangle(vec3_t v, const vec3_t m);

/* detect colors with components in 0-1 and scale them to 0-255 */
void normalize_color_format(vec3_t color);

/* command-line options */

enum class setting_source_t {
    DEFAULT = 0,
    MAP = 1,
    COMMANDLINE = 2
};

class lockable_setting_t {
protected:
    setting_source_t _source;
    std::vector<std::string> _names;
    
    lockable_setting_t(std::vector<std::string> names)
    : _source(setting_source_t::DEFAULT), _names(names) {
        assert(_names.size() > 0);
    }
    
    bool changeSource(setting_source_t newSource) {
        if (static_cast<int>(newSource) >= static_cast<int>(_source)) {
            _source = newSource;
            return true;
        }
        return false;
    }
    
public:
    const std::string &primaryName() const { return _names.at(0); }
    const std::vector<std::string> &names() const { return _names; }
    
    virtual void setStringValue(const std::string &str, bool locked = false) = 0;
    virtual std::string stringValue() const = 0;
    
    bool isChanged() const { return _source != setting_source_t::DEFAULT; }
    bool isLocked() const { return _source == setting_source_t::COMMANDLINE; }
    
    std::string sourceString() const {
        switch (_source) {
            case setting_source_t::DEFAULT: return "default";
            case setting_source_t::MAP: return "map";
            case setting_source_t::COMMANDLINE: return "commandline";
        }
    }
};

class lockable_bool_t : public lockable_setting_t {
private:
    bool _default, _value;
    
    void setBoolValueInternal(bool f, setting_source_t newsource) {
        if (changeSource(newsource)) {
            _value = f;
        }
    }
    
public:
    
    void setBoolValueLocked(bool f) {
        setBoolValueInternal(f, setting_source_t::COMMANDLINE);
    }
    
    void setBoolValue(bool f) {
        setBoolValueInternal(f, setting_source_t::MAP);
    }
    
    bool boolValue() const {
        return _value;
    }
    
    virtual void setStringValue(const std::string &str, bool locked = false) {
        int intval = std::stoi(str);
        
        const bool f = (intval != 0 && intval != -1); // treat 0 and -1 as false
        if (locked) setBoolValueLocked(f);
        else setBoolValue(f);
    }
    
    virtual std::string stringValue() const {
        return _value ? "1" : "0";
    }
    
    lockable_bool_t(std::vector<std::string> names, bool v)
    : lockable_setting_t(names), _default(v), _value(v) {}
    
    lockable_bool_t(std::string name, bool v)
    : lockable_bool_t(std::vector<std::string> { name }, v) {}
};

class lockable_vec_t : public lockable_setting_t {
private:
    float _default, _value, _min, _max;
    
    void setFloatInternal(float f, setting_source_t newsource) {
        if (changeSource(newsource)) {
            if (f < _min) {
                logprint("WARNING: '%s': %f is less than minimum value %f.\n",
                         primaryName().c_str(), f, _min);
                f = _min;
            }
            if (f > _max) {
                logprint("WARNING: '%s': %f is greater than maximum value %f.\n",
                         primaryName().c_str(), f, _max);
                f = _max;
            }
            _value = f;
        }
    }
    
public:
    bool boolValue() const {
        return static_cast<bool>(_value);
    }
    
    int intValue() const {
        return static_cast<int>(_value);
    }
    
    float floatValue() const {
        return _value;
    }
    
    void setFloatValue(float f) {
        setFloatInternal(f, setting_source_t::MAP);
    }
    
    void setFloatValueLocked(float f) {
        setFloatInternal(f, setting_source_t::COMMANDLINE);
    }
    
    virtual void setStringValue(const std::string &str, bool locked = false) {
        const float f = std::stof(str);
        if (locked) setFloatValueLocked(f);
        else setFloatValue(f);
    }
    
    virtual std::string stringValue() const {
        return std::to_string(_value);
    }
    
    lockable_vec_t(std::vector<std::string> names, float v,
                   float minval=-std::numeric_limits<float>::infinity(),
                   float maxval=std::numeric_limits<float>::infinity())
    : lockable_setting_t(names), _default(v), _value(v), _min(minval), _max(maxval) {
        // check the default value is valid
        assert(_min < _max);
        assert(_value >= _min);
        assert(_value <= _max);
    }
    
    lockable_vec_t(std::string name, float v,
                   float minval=-std::numeric_limits<float>::infinity(),
                   float maxval=std::numeric_limits<float>::infinity())
    : lockable_vec_t(std::vector<std::string> { name }, v, minval, maxval) {}
};

class lockable_string_t : public lockable_setting_t {
private:
    std::string _default, _value;
    
public:
    virtual void setStringValue(const std::string &str, bool locked = false) {
        if (changeSource(locked ? setting_source_t::COMMANDLINE : setting_source_t::MAP)) {
            _value = str;
        }
    }
    
    virtual std::string stringValue() const {
        return _value;
    }
    
    lockable_string_t(std::vector<std::string> names, std::string v)
    : lockable_setting_t(names), _default(v), _value(v) {}
    
    lockable_string_t(std::string name, std::string v)
    : lockable_string_t(std::vector<std::string> { name }, v) {}
};

enum class vec3_transformer_t {
    NONE,
    MANGLE_TO_VEC,
    NORMALIZE_COLOR_TO_255
};


class lockable_vec3_t : public lockable_setting_t {
private:
    vec3_t _default, _value;
    vec3_transformer_t _transformer;

    void transformVec3Value(const vec3_t val, vec3_t out) const {
        // apply transform
        switch (_transformer) {
            case vec3_transformer_t::NONE:
                VectorCopy(val, out);
                break;
            case vec3_transformer_t::MANGLE_TO_VEC:
                vec_from_mangle(out, val);
                break;
            case vec3_transformer_t::NORMALIZE_COLOR_TO_255:
                VectorCopy(val, out);
                normalize_color_format(out);
                break;
        }
    }
    
    void transformAndSetVec3Value(const vec3_t val, setting_source_t newsource) {
        if (changeSource(newsource)) {
            vec3_t tmp;
            transformVec3Value(val, tmp);
            VectorCopy(tmp, _value);
        }
    }
    
public:
    lockable_vec3_t(std::vector<std::string> names, vec_t a, vec_t b, vec_t c,
                    vec3_transformer_t t = vec3_transformer_t::NONE)
    : lockable_setting_t(names), _transformer(t)
    {
        vec3_t tmp = { a, b, c };
        transformVec3Value(tmp, _default);
        VectorCopy(_default, _value);
    }
    
    lockable_vec3_t(std::string name, vec_t a, vec_t b, vec_t c,
                    vec3_transformer_t t = vec3_transformer_t::NONE)
        : lockable_vec3_t(std::vector<std::string> { name }, a,b,c,t) {}
    
    const vec3_t *vec3Value() const {
        return &_value;
    }

    void setVec3Value(const vec3_t val) {
        transformAndSetVec3Value(val, setting_source_t::MAP);
    }
    
    void setVec3ValueLocked(const vec3_t val) {
        transformAndSetVec3Value(val, setting_source_t::COMMANDLINE);
    }
    
    virtual void setStringValue(const std::string &str, bool locked = false) {
        double vec[3] = { 0.0, 0.0, 0.0 };
        
        if (sscanf(str.c_str(), "%lf %lf %lf", &vec[0], &vec[1], &vec[2]) != 3) {
            logprint("WARNING: Not 3 values for %s\n", primaryName().c_str());
        }
        
        vec3_t vec3t;
        for (int i = 0; i < 3; ++i)
            vec3t[i] = vec[i];
        
        if (locked) setVec3ValueLocked(vec3t);
        else setVec3Value(vec3t);
    }
    
    virtual std::string stringValue() const {
        std::stringstream ss;
        ss << _value[0] << " "
           << _value[1] << " "
           << _value[2];
        return ss.str();
    }
};


extern int numDirtVectors;

// other flags

extern bool dirt_in_use;               // should any dirtmapping take place? set in SetupDirt

extern float fadegate;
extern int softsamples;
extern const vec3_t vec3_white;
extern float surflight_subdivide;
extern int sunsamples;

extern int dump_facenum;
extern bool dump_face;

extern int dump_vertnum;
extern bool dump_vert;

// settings dictionary

class settingsdict_t {
private:
    std::map<std::string, lockable_setting_t *> _settingsmap;
    std::vector<lockable_setting_t *> _allsettings;

public:
    settingsdict_t() {}
    
    settingsdict_t(std::vector<lockable_setting_t *> settings)
        : _allsettings(settings)
    {
        for (lockable_setting_t *setting : settings) {
            assert(setting->names().size() > 0);
            
            for (const auto &name : setting->names()) {
                assert(_settingsmap.find(name) == _settingsmap.end());
                
                _settingsmap[name] = setting;
            }
        }
    }
    
    lockable_setting_t *findSetting(std::string name) const {
        // strip off leading underscores
        if (name.find("_") == 0) {
            return findSetting(name.substr(1, name.size() - 1));
        }
        
        auto it = _settingsmap.find(name);
        if (it != _settingsmap.end()) {
            return it->second;
        } else {
            return nullptr;
        }
    }
    
    void setSetting(std::string name, std::string value, bool cmdline) {
        lockable_setting_t *setting = findSetting(name);
        if (setting == nullptr) {
            if (cmdline) {
                Error("Unrecognized command-line option '%s'\n", name.c_str());
            }
            return;
        }
        
        setting->setStringValue(value, cmdline);
    }
    
    void setSettings(const std::map<std::string, std::string> &epairs, bool cmdline) {
        for (const auto &epair : epairs) {
            setSetting(epair.first, epair.second, cmdline);
        }
    }
    
    const std::vector<lockable_setting_t *> &allSettings() const { return _allsettings; }
};


class modelinfo_t {
#define DEFAULT_PHONG_ANGLE 89.0f
    
public:
    const dmodel_t *model;
    float lightmapscale;
    vec3_t offset;

public:
    lockable_vec_t minlight, shadow, shadowself, dirt, phong, phong_angle;
    lockable_string_t minlight_exclude;
    lockable_vec3_t minlight_color;
    
    float getResolvedPhongAngle() const {
        const float s = phong_angle.floatValue();
        if (s != 0) {
            return s;
        }
        if (phong.boolValue()) {
            return DEFAULT_PHONG_ANGLE;
        }
        return 0;
    }
    
public:
    modelinfo_t(const dmodel_t *m, float lmscale) :
        model { m },
        lightmapscale { lmscale },
        minlight { "minlight", 0 },
        shadow { "shadow", 0 },
        shadowself { "shadowself", 0 },
        dirt { "dirt", 0 },
        phong { "phong", 0 },
        phong_angle { "phong_angle", 0 },
        minlight_exclude { "minlight_exclude", "" },
        minlight_color { "minlight_color", 255, 255, 255, vec3_transformer_t::NORMALIZE_COLOR_TO_255 }
    {
		VectorSet(offset, 0, 0, 0);
	}
    
    settingsdict_t settings() {
        return {{
            &minlight, &shadow, &shadowself, &dirt, &phong, &phong_angle,
            &minlight_exclude, &minlight_color
        }};
    }
};

//
// worldspawn keys / command-line settings
//

class globalconfig_t {
    using strings = std::vector<std::string>;
    
public:
    lockable_vec_t scaledist, rangescale, global_anglescale, lightmapgamma;
    lockable_bool_t addminlight;
    lockable_vec_t minlight;
    lockable_vec3_t minlight_color;
    
    /* dirt */
    lockable_bool_t globalDirt;          // apply dirt to all lights (unless they override it) + sunlight + minlight?
    lockable_vec_t dirtMode, dirtDepth, dirtScale, dirtGain, dirtAngle;
    
    lockable_bool_t minlightDirt;   // apply dirt to minlight?
    
    /* phong */
    lockable_bool_t phongallowed;
    
    /* bounce */
    lockable_bool_t bounce;
    lockable_vec_t bouncescale, bouncecolorscale;
    
    /* sunlight */
    
    lockable_vec_t sunlight;
    lockable_vec3_t sunlight_color;
    lockable_vec_t sun2;
    lockable_vec3_t sun2_color;
    lockable_vec_t sunlight2;
    lockable_vec3_t sunlight2_color;
    lockable_vec_t sunlight3;
    lockable_vec3_t sunlight3_color;
    lockable_vec_t sunlight_dirt;
    lockable_vec_t sunlight2_dirt;
    lockable_vec3_t sunvec;
    lockable_vec3_t sun2vec;
    lockable_vec_t sun_deviance;
    
    globalconfig_t() :
        scaledist {"dist", 1.0, 0.0f, 100.0f},
        rangescale {"range", 0.5f, 0.0f, 100.0f},
        global_anglescale {strings{"anglescale", "anglesense"}, 0.5, 0.0f, 1.0f},
        lightmapgamma {"gamma", 1.0, 0.0f, 100.0f},

        addminlight {"addmin", false},
        minlight {"light", 0},
        minlight_color {strings{"minlight_color", "mincolor"}, 255.0f, 255.0f, 255.0f, vec3_transformer_t::NORMALIZE_COLOR_TO_255},

        /* dirt */
        globalDirt {strings{"dirt", "dirty"}, false},
        dirtMode {"dirtmode", 0.0f},
        dirtDepth {"dirtdepth", 128.0f, 1.0f, std::numeric_limits<float>::infinity()},
        dirtScale {"dirtscale", 1.0f, 0.0f, 100.0f},
        dirtGain {"dirtgain", 1.0f, 0.0f, 100.0f},
        dirtAngle {"dirtangle", 88.0f, 0.0f, 90.0f},
        minlightDirt {"minlight_dirt", false},

        /* phong */
        phongallowed {"phong", true},

        /* bounce */
        bounce {"bounce", false},
        bouncescale {"bouncescale", 1.0f, 0.0f, 100.0f},
        bouncecolorscale {"bouncecolorscale", 0.0f, 0.0f, 1.0f},

        /* sun */
        sunlight         { "sunlight", 0.0f },                   /* main sun */
        sunlight_color  { "sunlight_color", 255.0f, 255.0f, 255.0f, vec3_transformer_t::NORMALIZE_COLOR_TO_255 },
        sun2             { "sun2", 0.0f },                   /* second sun */
        sun2_color      { "sun2_color", 255.0f, 255.0f, 255.0f, vec3_transformer_t::NORMALIZE_COLOR_TO_255 },
        sunlight2        { "sunlight2", 0.0f },                   /* top sky dome */
        sunlight2_color { strings{"sunlight2_color", "sunlight_color2"}, 255.0f, 255.0f, 255.0f, vec3_transformer_t::NORMALIZE_COLOR_TO_255 },
        sunlight3        { "sunlight3", 0.0f },                   /* bottom sky dome */
        sunlight3_color { strings{"sunlight3_color", "sunlight_color3"}, 255.0f, 255.0f, 255.0f, vec3_transformer_t::NORMALIZE_COLOR_TO_255 },
        sunlight_dirt    { "sunlight_dirt", 0.0f },
        sunlight2_dirt   { "sunlight2_dirt", 0.0f },
        sunvec          { strings{"sunlight_mangle", "sun_mangle"}, 0.0f, -90.0f, 0.0f, vec3_transformer_t::MANGLE_TO_VEC },  /* defaults to straight down */
        sun2vec         { "sun2_mangle", 0.0f, -90.0f, 0.0f, vec3_transformer_t::MANGLE_TO_VEC },  /* defaults to straight down */
        sun_deviance     { "sunlight_penumbra", 0.0f, 0.0f, 180.0f }
    {}
    
    settingsdict_t settings() {
        return {{
            &scaledist, &rangescale, &global_anglescale, &lightmapgamma,
            &addminlight,
            &minlight,
            &minlight_color,
            &globalDirt,
            &dirtMode, &dirtDepth, &dirtScale, &dirtGain, &dirtAngle,
            &minlightDirt,
            &phongallowed,
            &bounce,
            &bouncescale, &bouncecolorscale,
            &sunlight,
            &sunlight_color,
            &sun2,
            &sun2_color,
            &sunlight2,
            &sunlight2_color,
            &sunlight3,
            &sunlight3_color,
            &sunlight_dirt,
            &sunlight2_dirt,
            &sunvec,
            &sun2vec,
            &sun_deviance
        }};
    }
};

extern byte *filebase;
extern byte *lit_filebase;
extern byte *lux_filebase;

extern int oversample;
extern int write_litfile;
extern int write_luxfile;
extern qboolean onlyents;
extern qboolean scaledonly;
extern uint32_t *extended_texinfo_flags;
extern qboolean novisapprox;
extern bool nolights;

typedef enum {
    backend_bsp,
    backend_embree
} backend_t;
    
extern backend_t rtbackend;
extern qboolean surflight_dump;
extern char mapfilename[1024];

// public functions

lockable_setting_t *FindSetting(std::string name);
void SetGlobalSetting(std::string name, std::string value, bool cmdline);
void GetFileSpace(byte **lightdata, byte **colordata, byte **deluxdata, int size);
const modelinfo_t *ModelInfoForFace(const bsp2_t *bsp, int facenum);
const vec_t *GetSurfaceVertexNormal(const bsp2_t *bsp, const bsp2_dface_t *f, const int v);
const bsp2_dface_t *Face_EdgeIndexSmoothed(const bsp2_t *bsp, const bsp2_dface_t *f, const int edgeindex);
const std::vector<bouncelight_t> &BounceLights();
bool Leaf_HasSky(const bsp2_t *bsp, const bsp2_dleaf_t *leaf);

#endif /* __LIGHT_LIGHT_H__ */
