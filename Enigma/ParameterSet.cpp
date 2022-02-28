#include "IO.h"
#include "Error.h"
#include "Texture.h"
#include "ParameterSet.h"




namespace RayTrace
{
    // ParamSet Macros
#define ADD_PARAM_TYPE(T, vec) \
    (vec).emplace_back(new ParamSetItem<T>(name, std::move(values), nValues));
#define LOOKUP_PTR(vec)             \
    for (const auto &v : vec)       \
        if (v->name == name) {      \
            *nValues = v->nValues;  \
            v->lookedUp = true;     \
            return v->values.get(); \
        }                           \
    return nullptr
#define LOOKUP_ONE(vec)                           \
    for (const auto &v : vec)                     \
        if (v->name == name && v->nValues == 1) { \
            v->lookedUp = true;                   \
            return v->values[0];                  \
        }                                         \
    return d

// ParamSet Methods
    void ParamSet::AddFloat(const std::string& name,
        std::unique_ptr<float[]> values, int nValues) {
        EraseFloat(name);
        floats.emplace_back(
            new ParamSetItem<float>(name, std::move(values), nValues));
    }

    void ParamSet::AddInt(const std::string& name, std::unique_ptr<int[]> values,
        int nValues) {
        EraseInt(name);
        ADD_PARAM_TYPE(int, ints);
    }

    void ParamSet::AddBool(const std::string& name, std::unique_ptr<bool[]> values,
        int nValues) {
        EraseBool(name);
        ADD_PARAM_TYPE(bool, bools);
    }

    void ParamSet::AddPoint2f(const std::string& name,
        std::unique_ptr<Vector2f[]> values, int nValues) {
        ErasePoint2f(name);
        ADD_PARAM_TYPE(Vector2f, point2fs);
    }

    void ParamSet::AddVector2f(const std::string& name,
        std::unique_ptr<Vector2f[]> values, int nValues) {
        EraseVector2f(name);
        ADD_PARAM_TYPE(Vector2f, vector2fs);
    }

    void ParamSet::AddPoint3f(const std::string& name,
        std::unique_ptr<Vector3f[]> values, int nValues) {
        ErasePoint3f(name);
        ADD_PARAM_TYPE(Vector3f, point3fs);
    }

    void ParamSet::AddVector3f(const std::string& name,
        std::unique_ptr<Vector3f[]> values, int nValues) {
        EraseVector3f(name);
        ADD_PARAM_TYPE(Vector3f, vector3fs);
    }

    void ParamSet::AddNormal3f(const std::string& name,
        std::unique_ptr<Vector3f[]> values, int nValues) {
        EraseNormal3f(name);
        ADD_PARAM_TYPE(Vector3f, normals);
    }

    void ParamSet::AddRGBSpectrum(const std::string& name,
        std::unique_ptr<float[]> values, int nValues) {
        EraseSpectrum(name);
        assert(nValues % 3 == 0);
        nValues /= 3;
        std::unique_ptr<Spectrum[]> s(new Spectrum[nValues]);
        for (int i = 0; i < nValues; ++i) s[i] = Spectrum::FromRGB(&values[3 * i]);
        std::shared_ptr<ParamSetItem<Spectrum>> psi(
            new ParamSetItem<Spectrum>(name, std::move(s), nValues));
        spectra.push_back(psi);
    }

    void ParamSet::AddXYZSpectrum(const std::string& name,
        std::unique_ptr<float[]> values, int nValues) {
        EraseSpectrum(name);
        assert(nValues % 3 == 0);
        nValues /= 3;
        std::unique_ptr<Spectrum[]> s(new Spectrum[nValues]);
        for (int i = 0; i < nValues; ++i) s[i] = Spectrum::FromXYZ(&values[3 * i]);
        std::shared_ptr<ParamSetItem<Spectrum>> psi(
            new ParamSetItem<Spectrum>(name, std::move(s), nValues));
        spectra.push_back(psi);
    }

    void ParamSet::AddBlackbodySpectrum(const std::string& name,
        std::unique_ptr<float[]> values,
        int nValues) {
        EraseSpectrum(name);
        assert(nValues % 2 == 0);  // temperature (K), scale, ...
        nValues /= 2;
        std::unique_ptr<Spectrum[]> s(new Spectrum[nValues]);
        std::unique_ptr<float[]> v(new float[nCIESamples]);
        for (int i = 0; i < nValues; ++i) {
            BlackbodyNormalized(CIE_lambda, nCIESamples, values[2 * i], v.get());
            s[i] = values[2 * i + 1] *
                Spectrum::FromSampled(CIE_lambda, v.get(), nCIESamples);
        }
        std::shared_ptr<ParamSetItem<Spectrum>> psi(
            new ParamSetItem<Spectrum>(name, std::move(s), nValues));
        spectra.push_back(psi);
    }

    void ParamSet::AddSampledSpectrum(const std::string& name,
        std::unique_ptr<float[]> values,
        int nValues) {
        EraseSpectrum(name);
        assert(nValues % 2 == 0);
        nValues /= 2;
        std::unique_ptr<float[]> wl(new float[nValues]);
        std::unique_ptr<float[]> v(new float[nValues]);
        for (int i = 0; i < nValues; ++i) {
            wl[i] = values[2 * i];
            v[i] = values[2 * i + 1];
        }
        std::unique_ptr<Spectrum[]> s(new Spectrum[1]);
        s[0] = Spectrum::FromSampled(wl.get(), v.get(), nValues);
        std::shared_ptr<ParamSetItem<Spectrum>> psi(
            new ParamSetItem<Spectrum>(name, std::move(s), 1));
        spectra.push_back(psi);
    }

    void ParamSet::AddSampledSpectrumFiles(const std::string& name,
        const char** names, int nValues) {
        EraseSpectrum(name);
        std::unique_ptr<Spectrum[]> s(new Spectrum[nValues]);
        for (int i = 0; i < nValues; ++i) {
            std::string fn = AbsolutePath(ResolveFilename(names[i]));
            if (cachedSpectra.find(fn) != cachedSpectra.end()) {
                s[i] = cachedSpectra[fn];
                continue;
            }

            std::vector<float> vals;
            if (!ReadFloatFile(fn.c_str(), vals)) {
                assert(false && "Unable to read SPD file");
                s[i] = Spectrum(0.);
            }
            else {
                if (vals.size() % 2) {               
                       assert(false && "Extra value found in spectrum file");                       
                }
                std::vector<float> wls, v;
                for (size_t j = 0; j < vals.size() / 2; ++j) {
                    wls.push_back(vals[2 * j]);
                    v.push_back(vals[2 * j + 1]);
                }
                s[i] = Spectrum::FromSampled(&wls[0], &v[0], wls.size());
            }
            cachedSpectra[fn] = s[i];
        }

        std::shared_ptr<ParamSetItem<Spectrum>> psi(
            new ParamSetItem<Spectrum>(name, std::move(s), nValues));
        spectra.push_back(psi);
    }

    std::map<std::string, Spectrum> ParamSet::cachedSpectra;
    void ParamSet::AddString(const std::string& name,
        std::unique_ptr<std::string[]> values, int nValues) {
        EraseString(name);
        ADD_PARAM_TYPE(std::string, strings);
    }

    void ParamSet::AddTexture(const std::string& name, const std::string& value) {
        EraseTexture(name);
        std::unique_ptr<std::string[]> str(new std::string[1]);
        str[0] = value;
        std::shared_ptr<ParamSetItem<std::string>> psi(
            new ParamSetItem<std::string>(name, std::move(str), 1));
        textures.push_back(psi);
    }

    bool ParamSet::EraseInt(const std::string& n) {
        for (size_t i = 0; i < ints.size(); ++i)
            if (ints[i]->name == n) {
                ints.erase(ints.begin() + i);
                return true;
            }
        return false;
    }

    bool ParamSet::EraseBool(const std::string& n) {
        for (size_t i = 0; i < bools.size(); ++i)
            if (bools[i]->name == n) {
                bools.erase(bools.begin() + i);
                return true;
            }
        return false;
    }

    bool ParamSet::EraseFloat(const std::string& n) {
        for (size_t i = 0; i < floats.size(); ++i)
            if (floats[i]->name == n) {
                floats.erase(floats.begin() + i);
                return true;
            }
        return false;
    }

    bool ParamSet::ErasePoint2f(const std::string& n) {
        for (size_t i = 0; i < point2fs.size(); ++i)
            if (point2fs[i]->name == n) {
                point2fs.erase(point2fs.begin() + i);
                return true;
            }
        return false;
    }

    bool ParamSet::EraseVector2f(const std::string& n) {
        for (size_t i = 0; i < vector2fs.size(); ++i)
            if (vector2fs[i]->name == n) {
                vector2fs.erase(vector2fs.begin() + i);
                return true;
            }
        return false;
    }

    bool ParamSet::ErasePoint3f(const std::string& n) {
        for (size_t i = 0; i < point3fs.size(); ++i)
            if (point3fs[i]->name == n) {
                point3fs.erase(point3fs.begin() + i);
                return true;
            }
        return false;
    }

    bool ParamSet::EraseVector3f(const std::string& n) {
        for (size_t i = 0; i < vector3fs.size(); ++i)
            if (vector3fs[i]->name == n) {
                vector3fs.erase(vector3fs.begin() + i);
                return true;
            }
        return false;
    }

    bool ParamSet::EraseNormal3f(const std::string& n) {
        for (size_t i = 0; i < normals.size(); ++i)
            if (normals[i]->name == n) {
                normals.erase(normals.begin() + i);
                return true;
            }
        return false;
    }

    bool ParamSet::EraseSpectrum(const std::string& n) {
        for (size_t i = 0; i < spectra.size(); ++i)
            if (spectra[i]->name == n) {
                spectra.erase(spectra.begin() + i);
                return true;
            }
        return false;
    }

    bool ParamSet::EraseString(const std::string& n) {
        for (size_t i = 0; i < strings.size(); ++i)
            if (strings[i]->name == n) {
                strings.erase(strings.begin() + i);
                return true;
            }
        return false;
    }

    bool ParamSet::EraseTexture(const std::string& n) {
        for (size_t i = 0; i < textures.size(); ++i)
            if (textures[i]->name == n) {
                textures.erase(textures.begin() + i);
                return true;
            }
        return false;
    }

    float ParamSet::FindOneFloat(const std::string& name, float d) const {
        for (const auto& f : floats)
            if (f->name == name && f->nValues == 1) {
                f->lookedUp = true;
                return f->values[0];
            }
        return d;
    }

    const float* ParamSet::FindFloat(const std::string& name, int* n) const {
        for (const auto& f : floats)
            if (f->name == name) {
                *n = f->nValues;
                f->lookedUp = true;
                return f->values.get();
            }
        return nullptr;
    }

    const int* ParamSet::FindInt(const std::string& name, int* nValues) const {
        LOOKUP_PTR(ints);
    }

    const bool* ParamSet::FindBool(const std::string& name, int* nValues) const {
        LOOKUP_PTR(bools);
    }

    int ParamSet::FindOneInt(const std::string& name, int d) const {
        LOOKUP_ONE(ints);
    }

    bool ParamSet::FindOneBool(const std::string& name, bool d) const {
        LOOKUP_ONE(bools);
    }

    const Vector2f* ParamSet::FindPoint2f(const std::string& name,
        int* nValues) const {
        LOOKUP_PTR(point2fs);
    }

    Vector2f ParamSet::FindOnePoint2f(const std::string& name,
        const Vector2f& d) const {
        LOOKUP_ONE(point2fs);
    }

    const Vector2f* ParamSet::FindVector2f(const std::string& name,
        int* nValues) const {
        LOOKUP_PTR(vector2fs);
    }

    Vector2f ParamSet::FindOneVector2f(const std::string& name,
        const Vector2f& d) const {
        LOOKUP_ONE(vector2fs);
    }

    const Vector3f* ParamSet::FindPoint3f(const std::string& name,
        int* nValues) const {
        LOOKUP_PTR(point3fs);
    }

    Vector3f ParamSet::FindOnePoint3f(const std::string& name,
        const Vector3f& d) const {
        LOOKUP_ONE(point3fs);
    }

    const Vector3f* ParamSet::FindVector3f(const std::string& name,
        int* nValues) const {
        LOOKUP_PTR(vector3fs);
    }

    Vector3f ParamSet::FindOneVector3f(const std::string& name,
        const Vector3f& d) const {
        LOOKUP_ONE(vector3fs);
    }

    const Vector3f* ParamSet::FindNormal3f(const std::string& name,
        int* nValues) const {
        LOOKUP_PTR(normals);
    }

    Vector3f ParamSet::FindOneNormal3f(const std::string& name,
        const Vector3f& d) const {
        LOOKUP_ONE(normals);
    }

    const Spectrum* ParamSet::FindSpectrum(const std::string& name,
        int* nValues) const {
        LOOKUP_PTR(spectra);
    }

    Spectrum ParamSet::FindOneSpectrum(const std::string& name,
        const Spectrum& d) const {
        LOOKUP_ONE(spectra);
    }

    const std::string* ParamSet::FindString(const std::string& name,
        int* nValues) const {
        LOOKUP_PTR(strings);
    }

    std::string ParamSet::FindOneString(const std::string& name,
        const std::string& d) const {
        LOOKUP_ONE(strings);
    }

    std::string ParamSet::FindOneFilename(const std::string& name,
        const std::string& d) const {
        std::string filename = FindOneString(name, "");
        if (filename == "") return d;
        filename = AbsolutePath(ResolveFilename(filename));
        return filename;
    }

    std::string ParamSet::FindTexture(const std::string& name) const {
        std::string d = "";
        LOOKUP_ONE(textures);
    }

    void ParamSet::ReportUnused() const {
#define CHECK_UNUSED(v)                                                 \
    for (size_t i = 0; i < (v).size(); ++i)                             \
        if (!(v)[i]->lookedUp)                                          \
            Warning("Parameter \"%s\" not used", (v)[i]->name.c_str())
        CHECK_UNUSED(ints);
        CHECK_UNUSED(bools);
        CHECK_UNUSED(floats);
        CHECK_UNUSED(point2fs);
        CHECK_UNUSED(vector2fs);
        CHECK_UNUSED(point3fs);
        CHECK_UNUSED(vector3fs);
        CHECK_UNUSED(normals);
        CHECK_UNUSED(spectra);
        CHECK_UNUSED(strings);
        CHECK_UNUSED(textures);
    }

    void ParamSet::Clear() {
#define DEL_PARAMS(name) (name).erase((name).begin(), (name).end())
        DEL_PARAMS(ints);
        DEL_PARAMS(bools);
        DEL_PARAMS(floats);
        DEL_PARAMS(point2fs);
        DEL_PARAMS(vector2fs);
        DEL_PARAMS(point3fs);
        DEL_PARAMS(vector3fs);
        DEL_PARAMS(normals);
        DEL_PARAMS(spectra);
        DEL_PARAMS(strings);
        DEL_PARAMS(textures);
#undef DEL_PARAMS
    }

    //std::string ParamSet::ToString() const {
    //    std::string ret;
    //    size_t i;
    //    int j;
    //    std::string typeString;
    //    for (i = 0; i < ints.size(); ++i) {
    //        const std::shared_ptr<ParamSetItem<int>>& item = ints[i];
    //        typeString = "integer ";
    //        // Print _ParamSetItem_ declaration, determine how many to print
    //        int nPrint = item->nValues;
    //        ret += std::string("\"");
    //        ret += typeString;
    //        ret += item->name;
    //        ret += std::string("\"");
    //        ret += std::string(" [");
    //        for (j = 0; j < nPrint; ++j)
    //            ret += StringPrintf("%d ", item->values[j]);
    //        ret += std::string("] ");
    //    }
    //    for (i = 0; i < bools.size(); ++i) {
    //        const std::shared_ptr<ParamSetItem<bool>>& item = bools[i];
    //        typeString = "bool ";
    //        // Print _ParamSetItem_ declaration, determine how many to print
    //        int nPrint = item->nValues;
    //        ret += std::string("\"");
    //        ret += typeString;
    //        ret += item->name;
    //        ret += std::string("\"");
    //        ret += std::string(" [");
    //        for (j = 0; j < nPrint; ++j)
    //            ret += StringPrintf("\"%s\" ", item->values[j] ? "true" : "false");
    //        ret += std::string("] ");
    //    }
    //    for (i = 0; i < floats.size(); ++i) {
    //        const std::shared_ptr<ParamSetItem<float>>& item = floats[i];
    //        typeString = "float ";
    //        // Print _ParamSetItem_ declaration, determine how many to print
    //        int nPrint = item->nValues;
    //        ret += std::string("\"");
    //        ret += typeString;
    //        ret += item->name;
    //        ret += std::string("\"");
    //        ret += std::string(" [");
    //        for (j = 0; j < nPrint; ++j)
    //            ret += StringPrintf("%.8g ", item->values[j]);
    //        ret += std::string("] ");
    //    }
    //    for (i = 0; i < point2fs.size(); ++i) {
    //        const std::shared_ptr<ParamSetItem<Point2f>>& item = point2fs[i];
    //        typeString = "point2 ";
    //        // Print _ParamSetItem_ declaration, determine how many to print
    //        int nPrint = item->nValues;
    //        ret += std::string("\"");
    //        ret += typeString;
    //        ret += item->name;
    //        ret += std::string("\"");
    //        ret += std::string(" [");
    //        for (j = 0; j < nPrint; ++j)
    //            ret += StringPrintf("%.8g %.8g ", item->values[j].x,
    //                item->values[j].y);
    //        ret += std::string("] ");
    //    }
    //    for (i = 0; i < vector2fs.size(); ++i) {
    //        const std::shared_ptr<ParamSetItem<Vector2f>>& item = vector2fs[i];
    //        typeString = "vector2 ";
    //        // Print _ParamSetItem_ declaration, determine how many to print
    //        int nPrint = item->nValues;
    //        ret += std::string("\"");
    //        ret += typeString;
    //        ret += item->name;
    //        ret += std::string("\"");
    //        ret += std::string(" [");
    //        for (j = 0; j < nPrint; ++j)
    //            ret += StringPrintf("%.8g %.8g ", item->values[j].x,
    //                item->values[j].y);
    //        ret += std::string("] ");
    //    }
    //    for (i = 0; i < point3fs.size(); ++i) {
    //        const std::shared_ptr<ParamSetItem<Point3f>>& item = point3fs[i];
    //        typeString = "point3 ";
    //        // Print _ParamSetItem_ declaration, determine how many to print
    //        int nPrint = item->nValues;
    //        ret += std::string("\"");
    //        ret += typeString;
    //        ret += item->name;
    //        ret += std::string("\"");
    //        ret += std::string(" [");
    //        for (j = 0; j < nPrint; ++j)
    //            ret += StringPrintf("%.8g %.8g %.8g ", item->values[j].x,
    //                item->values[j].y, item->values[j].z);
    //        ret += std::string("] ");
    //    }
    //    for (i = 0; i < vector3fs.size(); ++i) {
    //        const std::shared_ptr<ParamSetItem<Vector3f>>& item = vector3fs[i];
    //        typeString = "vector3 ";
    //        // Print _ParamSetItem_ declaration, determine how many to print
    //        int nPrint = item->nValues;
    //        ret += std::string("\"");
    //        ret += typeString;
    //        ret += item->name;
    //        ret += std::string("\"");
    //        ret += std::string(" [");
    //        for (j = 0; j < nPrint; ++j)
    //            ret += StringPrintf("%.8g %.8g %.8g ", item->values[j].x,
    //                item->values[j].y, item->values[j].z);
    //        ret += std::string("] ");
    //    }
    //    for (i = 0; i < normals.size(); ++i) {
    //        const std::shared_ptr<ParamSetItem<Normal3f>>& item = normals[i];
    //        typeString = "normal ";
    //        // Print _ParamSetItem_ declaration, determine how many to print
    //        int nPrint = item->nValues;
    //        ret += std::string("\"");
    //        ret += typeString;
    //        ret += item->name;
    //        ret += std::string("\"");
    //        ret += std::string(" [");
    //        for (j = 0; j < nPrint; ++j)
    //            ret += StringPrintf("%.8g %.8g %.8g ", item->values[j].x,
    //                item->values[j].y, item->values[j].z);
    //        ret += std::string("] ");
    //    }
    //    for (i = 0; i < strings.size(); ++i) {
    //        const std::shared_ptr<ParamSetItem<std::string>>& item = strings[i];
    //        typeString = "string ";
    //        // Print _ParamSetItem_ declaration, determine how many to print
    //        int nPrint = item->nValues;
    //        ret += std::string("\"");
    //        ret += typeString;
    //        ret += item->name;
    //        ret += std::string("\"");
    //        ret += std::string(" [");
    //        for (j = 0; j < nPrint; ++j)
    //            ret += StringPrintf("\"%s\" ", item->values[j].c_str());
    //        ret += std::string("] ");
    //    }
    //    for (i = 0; i < textures.size(); ++i) {
    //        const std::shared_ptr<ParamSetItem<std::string>>& item = textures[i];
    //        typeString = "texture ";
    //        // Print _ParamSetItem_ declaration, determine how many to print
    //        int nPrint = item->nValues;
    //        ret += std::string("\"");
    //        ret += typeString;
    //        ret += item->name;
    //        ret += std::string("\"");
    //        ret += std::string(" [");
    //        for (j = 0; j < nPrint; ++j)
    //            ret += StringPrintf("\"%s\" ", item->values[j].c_str());
    //        ret += std::string("] ");
    //    }
    //    for (i = 0; i < spectra.size(); ++i) {
    //        const std::shared_ptr<ParamSetItem<Spectrum>>& item = spectra[i];
    //        typeString = "color ";
    //        // Print _ParamSetItem_ declaration, determine how many to print
    //        int nPrint = item->nValues;
    //        ret += std::string("\"");
    //        ret += typeString;
    //        ret += item->name;
    //        ret += std::string("\"");
    //        ret += std::string(" [");
    //        for (j = 0; j < nPrint; ++j) {
    //            float rgb[3];
    //            item->values[j].ToRGB(rgb);
    //            ret += StringPrintf("%.8g %.8g %.8g ", rgb[0], rgb[1], rgb[2]);
    //        }
    //        ret += std::string("] ");
    //    }
    //    return ret;
    //}

    //static int print(int i) { return printf("%d ", i); }
    //static int print(bool v) {
    //    return v ? printf("\"true\" ") : printf("\"false\" ");
    //}
    //static int print(float f) {
    //    if ((int)f == f)
    //        return printf("%d ", (int)f);
    //    else
    //        return printf("%.9g ", f);
    //}
    //static int print(const Point2f& p) {
    //    int np = print(p.x);
    //    return np + print(p.y);
    //}
    //static int print(const Vector2f& v) {
    //    int np = print(v.x);
    //    return np + print(v.y);
    //}
    //static int print(const Point3f& p) {
    //    int np = print(p.x);
    //    np += print(p.y);
    //    return np + print(p.z);
    //}
    //static int print(const Vector3f& v) {
    //    int np = print(v.x);
    //    np += print(v.y);
    //    return np + print(v.z);
    //}
    //static int print(const Normal3f& n) {
    //    int np = print(n.x);
    //    np += print(n.y);
    //    return np + print(n.z);
    //}
    //static int print(const std::string& s) { return printf("\"%s\" ", s.c_str()); }
    //static int print(const Spectrum& s) {
    //    float rgb[3];
    //    s.ToRGB(rgb);
    //    int np = print(rgb[0]);
    //    np += print(rgb[1]);
    //    return np + print(rgb[2]);
    //}

    //template <typename T>
    //static void printItems(
    //    const char* type, int indent,
    //    const std::vector<std::shared_ptr<ParamSetItem<T>>>& items) {
    //    for (const auto& item : items) {
    //        int np = printf("\n%*s\"%s %s\" [ ", indent + 8, "", type,
    //            item->name.c_str());
    //        for (int i = 0; i < item->nValues; ++i) {
    //            np += print(item->values[i]);
    //            if (np > 80 && i < item->nValues - 1)
    //                np = printf("\n%*s", indent + 8, "");
    //        }
    //        printf("] ");
    //    }
    //}

    //void ParamSet::Print(int indent) const {
    //    printItems("integer", indent, ints);
    //    printItems("bool", indent, bools);
    //    printItems("float", indent, floats);
    //    printItems("point2", indent, point2fs);
    //    printItems("vector2", indent, vector2fs);
    //    printItems("point", indent, point3fs);
    //    printItems("vector", indent, vector3fs);
    //    printItems("normal", indent, normals);
    //    printItems("string", indent, strings);
    //    printItems("texture", indent, textures);
    //    printItems("rgb", indent, spectra);
    //}

    // TextureParams Method Definitions
    std::shared_ptr<Texture<Spectrum>> TextureParams::GetSpectrumTexture(
        const std::string& n, const Spectrum& def) const {
        std::shared_ptr<Texture<Spectrum>> tex = GetSpectrumTextureOrNull(n);
        if (tex)
            return tex;
        else
            return std::make_shared<ConstantTexture<Spectrum>>(def);
    }

    std::shared_ptr<Texture<Spectrum>> TextureParams::GetSpectrumTextureOrNull(
        const std::string& n) const {
        // Check the shape parameters first.
        std::string name = geomParams.FindTexture(n);
        if (name.empty()) {
            int count;
            const Spectrum* s = geomParams.FindSpectrum(n, &count);
            if (s) {
                if (count > 1)
                    Warning("Ignoring excess values provided with parameter \"%s\"",
                        n.c_str());
                return std::make_shared<ConstantTexture<Spectrum>>(*s);
            }

            name = materialParams.FindTexture(n);
            if (name.empty()) {
                int count;
                const Spectrum* s = materialParams.FindSpectrum(n, &count);
                if (s) {
                    if (count > 1)
                        Warning("Ignoring excess values provided with parameter \"%s\"",
                            n.c_str());
                    return std::make_shared<ConstantTexture<Spectrum>>(*s);
                }
            }

            if (name.empty())
                return nullptr;
        }

        // We have a texture name, from either the shape or the material's
        // parameters.
        if (spectrumTextures.find(name) != spectrumTextures.end())
            return spectrumTextures[name];
        else {
            Error("Couldn't find spectrum texture named \"%s\" for parameter \"%s\"",
                name.c_str(), n.c_str());
            return nullptr;
        }
    }

    std::shared_ptr<Texture<float>> TextureParams::GetFloatTexture(
        const std::string& n, float def) const {
        std::shared_ptr<Texture<float>> tex = GetFloatTextureOrNull(n);
        if (tex)
            return tex;
        else
            return std::make_shared<ConstantTexture<float>>(def);
    }

    std::shared_ptr<Texture<float>> TextureParams::GetFloatTextureOrNull(
        const std::string& n) const {
        // Check the shape parameters first.
        std::string name = geomParams.FindTexture(n);
        if (name.empty()) {
            int count;
            const float* s = geomParams.FindFloat(n, &count);
            if (s) {
                if (count > 1)
                    Warning("Ignoring excess values provided with parameter \"%s\"",
                        n.c_str());
                return std::make_shared<ConstantTexture<float>>(*s);
            }

            name = materialParams.FindTexture(n);
            if (name.empty()) {
                int count;
                const float* s = materialParams.FindFloat(n, &count);
                if (s) {
                    if (count > 1)
                        Warning("Ignoring excess values provided with parameter \"%s\"",
                            n.c_str());
                    return std::make_shared<ConstantTexture<float>>(*s);
                }
            }

            if (name.empty())
                return nullptr;
        }

        // We have a texture name, from either the shape or the material's
        // parameters.
        if (floatTextures.find(name) != floatTextures.end())
            return floatTextures[name];
        else {
            Error("Couldn't find float texture named \"%s\" for parameter \"%s\"",
                name.c_str(), n.c_str());
            return nullptr;
        }
    }

    template <typename T> static void
        reportUnusedMaterialParams(
            const std::vector<std::shared_ptr<ParamSetItem<T>>>& mtl,
            const std::vector<std::shared_ptr<ParamSetItem<T>>>& geom) {
        for (const auto& param : mtl) {
            if (param->lookedUp)
                continue;

            // Don't complain about any unused material parameters if their
            // values were provided by a shape parameter.
            if (std::find_if(geom.begin(), geom.end(),
                [&param](const std::shared_ptr<ParamSetItem<T>>& gp) {
                    return gp->name == param->name;
                }) == geom.end())
                Warning("Parameter \"%s\" not used", param->name.c_str());
        }
    }

    void TextureParams::ReportUnused() const {
        geomParams.ReportUnused();
        reportUnusedMaterialParams(materialParams.ints, geomParams.ints);
        reportUnusedMaterialParams(materialParams.bools, geomParams.bools);
        reportUnusedMaterialParams(materialParams.floats, geomParams.floats);
        reportUnusedMaterialParams(materialParams.point2fs, geomParams.point2fs);
        reportUnusedMaterialParams(materialParams.vector2fs, geomParams.vector2fs);
        reportUnusedMaterialParams(materialParams.point3fs, geomParams.point3fs);
        reportUnusedMaterialParams(materialParams.vector3fs, geomParams.vector3fs);
        reportUnusedMaterialParams(materialParams.normals, geomParams.normals);
        reportUnusedMaterialParams(materialParams.spectra, geomParams.spectra);
        reportUnusedMaterialParams(materialParams.strings, geomParams.strings);
        reportUnusedMaterialParams(materialParams.textures, geomParams.textures);
    }

}

