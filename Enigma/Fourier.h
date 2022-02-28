#pragma once
#include <fstream>
#include "Defines.h"
#include "Interpolation.h"
#include "Error.h"

namespace RayTrace
{

#pragma region FourierMaterial

    struct FourierBSDFTable
    {
        struct Header {
            char     magic[8];
            int      flags;
            int      nMu;
            int      nCoeffs;
            int      mMax;
            int      nChannels;
            int      nBases;
            int      pad0[3];
            float    eta;
            int      pad1[4];

            bool verify() const
            {
                static const char header_exp[8] = { 'S', 'C', 'A', 'T', 'F', 'U', 'N', '\x01' };
                if (memcmp(magic, header_exp, 8) != 0)
                    return false;
                if (flags != 1 || (nChannels != 1 && nChannels != 3) || nBases != 1)
                    return false;
                return true;
            }

           
        };

        static const int size = sizeof(Header);
        // FourierBSDFTable Public Data
        Header      header;

        FloatVector mu;
        FloatVector a;
        FloatVector a0;
        FloatVector cdf;
        FloatVector recip;
        IntVector   m;
        IntVector   aOffset;


        ~FourierBSDFTable() {

        }

        template<typename T>
        static inline bool ReadType(std::ifstream& _file, T* _dst, std::size_t _count)
        {
            const auto numBytes = sizeof(T) * _count;
            _file.read(reinterpret_cast<char*>(_dst), numBytes);
            return numBytes == _file.gcount();

        }

        // FourierBSDFTable Public Methods
        static bool Read(const std::string& _filename, FourierBSDFTable* table)
        {

            std::ifstream fileIn(_filename, std::ios::binary);
            if (!fileIn.is_open()) {
                Error("Unable to open tabulated BSDF file \"%s\"", _filename.c_str());
                return false;
            }

            auto& header = table->header;

            if (!ReadType<FourierBSDFTable::Header>(fileIn, &header, 1))
            {
                Error("Error reading Fourier Header");
                return false;
            }

            if (!table->header.verify()) {
                Error("Invalid Format");
                return false;
            }

            auto numElems = header.nMu;
            auto numElemsSqr = numElems * numElems;

            table->mu.resize(numElems);
            table->a.resize(header.nCoeffs);

            table->recip.resize(header.mMax);
            table->a0.resize(numElemsSqr);
            table->aOffset.resize(numElemsSqr);
            table->m.resize(numElemsSqr);
            table->cdf.resize(numElemsSqr);       

            IntVector offsetAndLength(numElemsSqr * 2);

            ReadType(fileIn, table->mu.data(), table->mu.size());
            ReadType(fileIn, table->cdf.data(), table->cdf.size());
            ReadType(fileIn, offsetAndLength.data(), offsetAndLength.size());
            ReadType(fileIn, table->a.data(), table->a.size());


            for (int i = 0; i < numElems * numElems; ++i) {
                int offset = offsetAndLength[2 * i],
                    length = offsetAndLength[2 * i + 1];
                table->aOffset[i] = offset;
                table->m[i] = length;
                table->a0[i] = length > 0 ? table->a[offset] : 0.f;
            }

            for (int i = 0; i < header.mMax; ++i)
                table->recip[i] = 1 / (float)i;

            return true;
        }


        const float* GetAk(int offsetI, int offsetO, int* mptr) const {
            *mptr = m[offsetO * header.nMu + offsetI];
            return a.data() + aOffset[offsetO * header.nMu + offsetI];
        }


        bool GetWeightsAndOffset(float cosTheta, int* offset, float weights[4]) const
        {
            return CatmullRomWeights(header.nMu, mu.data(), cosTheta, offset, weights);
        }
    };

   


#pragma endregion
}