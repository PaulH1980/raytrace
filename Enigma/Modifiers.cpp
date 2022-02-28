#include "Context.h"
#include "HWShader.h"
#include "HWMaterial.h"
#include "ModifierStack.h"
#include "Modifiers.h"
#include "Geometry.h"

namespace RayTrace
{
  
    inline sUnprocessedFace FlipFace(sUnprocessedFace _face)
    {
        _face.Flip();
        return _face;
    }

    inline bool Triangulate(GeometryBase& _object)
    {
        for (auto& face : _object.m_unprocessed)
        {
            auto e1 = face.m_verts[1] - face.m_verts[0];
            auto e2 = face.m_verts[2] - face.m_verts[0];
            
            auto normal = Normalize(Cross(e1, e2));
            for (int i = 0; i < face.m_verts.size(); i += 2) {

                int next     = (i + 1) % face.m_verts.size();
                int nextnext = (i + 2) % face.m_verts.size();

                _object.m_verts.push_back(face.m_verts[i]);
                _object.m_uvs.push_back(face.m_uvs[i]);
                _object.m_normals.push_back(normal);

                _object.m_verts.push_back(face.m_verts[next]);
                _object.m_uvs.push_back(face.m_uvs[next]);
                _object.m_normals.push_back(normal);

                _object.m_verts.push_back(face.m_verts[nextnext]);
                _object.m_uvs.push_back(face.m_uvs[nextnext]);
                _object.m_normals.push_back(normal);
            }
        }
        return true;
        
    }



   
    /// <summary>
    /// ModifierBase Implementation
    /// </summary>
    /// <param name="_pContext"></param>
    ModifierBase::ModifierBase(Context* _pContext)
        : ObjectBase(_pContext)          
    {
      
    }

    void ModifierBase::ObjectModified()
    {
        m_pStack->Updated(this);
    }

    ObjPropVector ModifierBase::GetProperties()
    {
        if (!ContainsProperty( GetTypeNameStatic() ) )
        {
            auto pProperty = std::make_unique<PropertyData>();
			pProperty->m_name = GetTypeNameStatic();
			pProperty->AddProperty(
				{ Properties<Vector3f>(&m_dimensions, this, true, DIM_MIN, DIM_MAX) , "Dimensions" }
			);
            ObjectBase::AddPropertyData( std::move(pProperty) );
        }

        auto retVal = ObjectBase::GetProperties();
        retVal.push_back(ObjectBase::GetPropertyData(GetTypeNameStatic()));
        return retVal;     
    }

    GeometryBase* ModifierBase::GetGeometry() const
    {
        return m_pStack->GetGeometry();
    }


   void ModifierBase::SetModifierStack(ModifierStack* _pStack) 
   { 
       m_pStack = _pStack; 
   }

    ModifierBase::ModifierBase(const ModifierBase& _rhs)
        : ObjectBase(_rhs)
        , m_dimensions(_rhs.m_dimensions)
    {

    }


    /// <summary>
    /// PlaneModifier Implementation
    /// </summary>
    /// <param name="_pContext"></param>
    PlaneModifier::PlaneModifier(Context* _pContext)
        : ModifierBase(_pContext)
      
    {
        m_name = "Create Plane";       
    }


    PlaneModifier::PlaneModifier(const PlaneModifier& _rhs)
        : ModifierBase(_rhs)
        , m_subDivs(_rhs.m_subDivs)
    {
    }

    void PlaneModifier::Apply()
    {
        auto* pObject = GetGeometry();


        const float xInc = m_dimensions.x / m_subDivs.x;
        const float zInc = m_dimensions.z / m_subDivs.y;
       

        const Vector2f uvwInc = Inverted(Vector2f(m_subDivs));


        const float xStart = -m_dimensions.x * 0.5f;
        const float zStart = -m_dimensions.z * 0.5f;
      
        const float xEnd = m_dimensions.x * 0.5f;
        const float zEnd = m_dimensions.z * 0.5f;
       


        for (int x = 0; x < m_subDivs.x; ++x)
        {
            float xCur = x * xInc + xStart;
            float xNext = xCur + xInc;
            float uCur = x * uvwInc.x;
            float uNext = uCur + uvwInc.x;
            { //bottom & top
                

                for (int z = 0; z < m_subDivs.y; z++) {
                    sUnprocessedFace top;
                    float zCur = z * zInc + zStart;
                    float zNext = zCur + zInc;

                    float vCur = z * uvwInc.y;
                    float vNext = vCur + uvwInc.y;


                    top.m_verts.emplace_back(xCur, 0.f,  zCur);
                    top.m_verts.emplace_back(xCur, 0.f,  zNext);
                    top.m_verts.emplace_back(xNext, 0.f, zNext);
                    top.m_verts.emplace_back(xNext, 0.f,  zCur);

                    top.m_uvs.emplace_back(uCur,  1.0f - vCur);
                    top.m_uvs.emplace_back(uCur,  1.0f - vNext);
                    top.m_uvs.emplace_back(uNext, 1.0f - vNext);
                    top.m_uvs.emplace_back(uNext, 1.0f - vCur);

                    pObject->m_unprocessed.push_back(top);
                }
              
            }
        }
        Triangulate(*pObject);
    }

    ObjPropVector PlaneModifier::GetProperties()
    {
		if (!ContainsProperty(GetTypeNameStatic()))
		{
			auto pProperty = std::make_unique<PropertyData>();
			pProperty->m_name = GetTypeNameStatic();
			pProperty->AddProperty(
                { Properties<Vector2i>(&m_subDivs, this, true, SD_MIN2, SD_MAX2), "Divisions" }
			);
			ObjectBase::AddPropertyData( std::move(pProperty));
		}

		auto retVal = ModifierBase::GetProperties();
		retVal.push_back(ObjectBase::GetPropertyData(GetTypeNameStatic()));
		return retVal;      
    }


    PlaneModifier* PlaneModifier::Clone() const
    {
        auto newMod = new PlaneModifier(*this);
        return newMod;   
    }



    /// <summary>
    /// DiskModifier Implementation
    /// </summary>
    /// <param name="_pContext"></param>
    DiskModifier::DiskModifier(Context* _pContext)
        : ModifierBase(_pContext)
      
    {
        m_name = "Create Disk";  
    }

    DiskModifier::DiskModifier(const DiskModifier& _rhs)
        : ModifierBase(_rhs)
        , m_numSides(_rhs.m_numSides)
        , m_numStacks(_rhs.m_numStacks)
    {
    }


    void DiskModifier::Apply()
    {
       
        auto* pObject = GetGeometry();


        const Vector2f start = { m_dimensions.x * -0.5, m_dimensions.z * -0.5f };
        const Vector2f end = { m_dimensions.x * 0.5, m_dimensions.z * 0.5f };
        const float sideInc = ToRadians(360.0f) / m_numSides;

        for (int side = 0; side < m_numSides; ++side) {
            const float xCur  = cosf(side * sideInc) * m_dimensions.x * 0.5f;
            const float zCur  = sinf(side * sideInc) * m_dimensions.z * 0.5f;
            const float xNext = cosf(side * sideInc + sideInc) * m_dimensions.x * 0.5f;
            const float zNext = sinf(side * sideInc + sideInc) * m_dimensions.z * 0.5f;

            const auto sideCur  = Vector2f{ xCur, zCur };
            const auto sideNext = Vector2f{ xNext, zNext };

            const auto lengthCur  = Length(sideCur);
            const auto lengthNext = Length(sideNext);

            const Vector2f curDir     = -1.0f * sideCur / lengthCur;     
            const Vector2f nextDir    = -1.0f * sideNext / lengthNext;

            const float    curLengthInc  = lengthCur / m_numStacks;
            const float    nextLengthInc = lengthNext / m_numStacks;

            auto NormalizedCoordinate = [](auto _input, auto _min, auto _max)
            {
                const auto range = _max - _min;
                return (_input - _min) / range;
            };


            for (int stack = 0; stack < m_numStacks; ++stack)
            {
                const Vector2f a = sideCur + curDir * (curLengthInc * stack + curLengthInc);
                const Vector2f b = sideCur + curDir * (curLengthInc * stack );
                const Vector2f c = sideNext + nextDir * (nextLengthInc * stack);
                const Vector2f d = sideNext + nextDir * (nextLengthInc * stack + nextLengthInc);



                sUnprocessedFace face;
                face.m_verts.emplace_back( a.x, 0.0f, a.y );
                face.m_verts.emplace_back( b.x, 0.0f, b.y );
                face.m_verts.emplace_back( c.x, 0.0f, c.y );
                face.m_verts.emplace_back( d.x, 0.0f, d.y );
                
                const auto uvStackCur  = NormalizedCoordinate(a , start, end );
                const auto uvSideCur   = NormalizedCoordinate(b , start, end );
                const auto uvSideNext  = NormalizedCoordinate(c , start, end );
                const auto uvStackNext = NormalizedCoordinate(d , start, end );

                face.m_uvs.emplace_back( uvStackCur  .x , 1.0f - uvStackCur  .y);
                face.m_uvs.emplace_back( uvSideCur   .x , 1.0f - uvSideCur   .y);
                face.m_uvs.emplace_back( uvSideNext  .x , 1.0f - uvSideNext  .y);
                face.m_uvs.emplace_back( uvStackNext .x , 1.0f - uvStackNext .y);
                face.Flip();
                pObject->m_unprocessed.push_back(face);
            }

        }
        Triangulate(*pObject);
    }

    ObjPropVector DiskModifier::GetProperties()
    {
		if (!ContainsProperty(GetTypeNameStatic()))
		{
			auto pProperty = std::make_unique<PropertyData>();
			pProperty->m_name = GetTypeNameStatic();
			pProperty->AddProperty(
                { Properties<int>(&m_numSides, this, true, SIDES_MIN, SD_MAX), "Sides" }
			);
			pProperty->AddProperty(
                { Properties<int>(&m_numStacks, this, true, STACKS_MIN, SD_MAX), "Stacks" }
			);
			ObjectBase::AddPropertyData( std::move(pProperty));
		}

		auto retVal = ModifierBase::GetProperties();
		retVal.push_back(ObjectBase::GetPropertyData(GetTypeNameStatic()));
		return retVal;           
    }

    DiskModifier* DiskModifier::Clone() const
    {
        auto newMod = new  DiskModifier(*this);
        return newMod;
    }

   
       

    /// <summary>
    /// CubeModifier Implementation
    /// </summary>
    /// <param name="_pContext"></param>
    CubeModifier::CubeModifier(Context* _pContext)
        : ModifierBase(_pContext)
    {
        m_name = "Create Cube";      
    }

    CubeModifier::CubeModifier(const CubeModifier& _rhs) 
        : ModifierBase(_rhs)
        , m_subDivs(_rhs.m_subDivs) 
    {
    }

    void CubeModifier::Apply()
{
        auto* pObject = GetGeometry();
        
        
        const float xInc = m_dimensions.x / m_subDivs.x;
        const float yInc = m_dimensions.y / m_subDivs.y;
        const float zInc = m_dimensions.z / m_subDivs.z;

        const Vector3f uvwInc = Inverted( Vector3f(m_subDivs) );
        

        const float xStart   = -m_dimensions.x * 0.5f;
        const float yStart   = -m_dimensions.y * 0.5f;
        const float zStart   = -m_dimensions.z * 0.5f;    

        const float xEnd     = m_dimensions.x * 0.5f;
        const float yEnd     = m_dimensions.y * 0.5f;
        const float zEnd     = m_dimensions.z * 0.5f;



        for (int x = 0; x < m_subDivs.x; ++x)
        {
            float xCur  = x * xInc + xStart;
            float xNext = xCur + xInc;          
            float uCur  = x * uvwInc.x;
            float uNext = uCur + uvwInc.x;
            { //bottom & top
              
               
                for (int z = 0; z < m_subDivs.z; z++) {
                    sUnprocessedFace top,
                                     bottom;

                    float zCur  = z * zInc + zStart;
                    float zNext = zCur + zInc;
                    float vCur  = z * uvwInc.z;
                    float vNext = vCur + uvwInc.z;

                    bottom.m_verts.emplace_back(xCur,  yStart, zCur);
                    bottom.m_verts.emplace_back(xCur,  yStart, zNext);
                    bottom.m_verts.emplace_back(xNext, yStart, zNext);
                    bottom.m_verts.emplace_back(xNext, yStart, zCur);


                    bottom.m_uvs.emplace_back(uCur,  1.0f - vCur);
                    bottom.m_uvs.emplace_back(uCur,  1.0f - vNext);
                    bottom.m_uvs.emplace_back(uNext, 1.0f - vNext);
                    bottom.m_uvs.emplace_back(uNext, 1.0f - vCur);

                    top.m_verts.emplace_back(xCur,  yEnd,  zCur);
                    top.m_verts.emplace_back(xCur,  yEnd,  zNext);
                    top.m_verts.emplace_back(xNext, yEnd,  zNext);
                    top.m_verts.emplace_back(xNext, yEnd,  zCur);

                    top.m_uvs.emplace_back( uCur,  1.0f - vCur);
                    top.m_uvs.emplace_back( uCur,  1.0f - vNext);
                    top.m_uvs.emplace_back( uNext, 1.0f - vNext);
                    top.m_uvs.emplace_back( uNext, 1.0f - vCur);

                    bottom.Flip();
                    pObject->m_unprocessed.push_back(bottom);
                    pObject->m_unprocessed.push_back(top);
                   
                }
                
              
            }

            {//front bback
           


                for (int y = 0; y < m_subDivs.y; y++) {
                    float yCur = y * yInc + yStart;
                    float yNext = yCur + yInc;
                    float vCur = y * uvwInc.y;
                    float vNext = vCur + uvwInc.y;

                    sUnprocessedFace front,
                        back;
                    back.m_verts.emplace_back(xCur,  yCur,  zStart  );
                    back.m_verts.emplace_back(xCur,  yNext, zStart  );
                    back.m_verts.emplace_back(xNext, yNext, zStart  );
                    back.m_verts.emplace_back(xNext, yCur , zStart  );

                    back.m_uvs.emplace_back(1.0f - uCur, vCur);
                    back.m_uvs.emplace_back(1.0f - uCur, vNext);
                    back.m_uvs.emplace_back(1.0f - uNext, vNext);
                    back.m_uvs.emplace_back(1.0f - uNext, vCur);

                    front.m_verts.emplace_back(xCur,  yCur,   -zStart);
                    front.m_verts.emplace_back(xCur,  yNext,  -zStart);
                    front.m_verts.emplace_back(xNext, yNext, -zStart);
                    front.m_verts.emplace_back(xNext, yCur,  -zStart);

                    front.m_uvs.emplace_back(uCur, vCur);
                    front.m_uvs.emplace_back(uCur, vNext);
                    front.m_uvs.emplace_back(uNext, vNext);
                    front.m_uvs.emplace_back(uNext, vCur);

                    front.Flip();

                    pObject->m_unprocessed.push_back(front);
                    pObject->m_unprocessed.push_back(back);
                }

              //  front.Flip();
            

            }           
        }
        
        for (int y = 0; y < m_subDivs.y; y++) {
            float yCur = y * yInc + yStart;
            float yNext = yCur + yInc;
            float vCur  = y * uvwInc.y;
            float vNext = vCur + uvwInc.y;

            for (int z = 0; z < m_subDivs.z; z++) {
                float zCur  = z * zInc + zStart;
                float zNext = zCur + zInc;
                float uCur  = z * uvwInc.z;
                float uNext = uCur + uvwInc.z;


                sUnprocessedFace left, right;

                left.m_verts.emplace_back(xStart, yCur,  zCur   );
                left.m_verts.emplace_back(xStart, yNext, zCur  );
                left.m_verts.emplace_back(xStart, yNext, zNext );
                left.m_verts.emplace_back(xStart, yCur,  zNext  );

                left.m_uvs.emplace_back( uCur,   vCur);
                left.m_uvs.emplace_back( uCur,   vNext);
                left.m_uvs.emplace_back( uNext,  vNext);
                left.m_uvs.emplace_back( uNext,  vCur);

                right.m_verts.emplace_back(xEnd, yCur,  zCur  );
                right.m_verts.emplace_back(xEnd, yNext, zCur  );
                right.m_verts.emplace_back(xEnd, yNext, zNext );
                right.m_verts.emplace_back(xEnd, yCur,  zNext );

                right.m_uvs.emplace_back(1.0f - uCur, vCur);
                right.m_uvs.emplace_back(1.0f - uCur, vNext);
                right.m_uvs.emplace_back(1.0f - uNext, vNext);
                right.m_uvs.emplace_back(1.0f - uNext, vCur);

                left.Flip();
                pObject->m_unprocessed.push_back(left);
                pObject->m_unprocessed.push_back(right);
            }
          
        }
        Triangulate(*pObject);
    }

    ObjPropVector CubeModifier::GetProperties()
    {
        if (!ContainsProperty(GetTypeNameStatic()))
        {
            auto pProperty = std::make_unique<PropertyData>();
            pProperty->m_name = GetTypeNameStatic();
            pProperty->AddProperty(
                { Properties<Vector3i>(&m_subDivs, this, true, SD_MIN3, SD_MAX3), "Divisions" }
            );         
            ObjectBase::AddPropertyData(std::move(pProperty));
        }

        auto retVal = ModifierBase::GetProperties();
        retVal.push_back(ObjectBase::GetPropertyData(GetTypeNameStatic()));
        return retVal;    
    }

    CubeModifier* CubeModifier::Clone() const
    {
        auto newMod = new  CubeModifier(*this);
        return newMod;
    }

   
    /// <summary>
    /// SphereModifier Implementation
    /// </summary>
    /// <param name="_pContext"></param>
    SphereModifier::SphereModifier(Context* _pContext)
        : ModifierBase(_pContext)
    {
        m_name = "Create Sphere";
    }


    SphereModifier::SphereModifier(const SphereModifier& _rhs) 
        : ModifierBase(_rhs)
        , m_numSides(_rhs.m_numSides)
        , m_numStacks(_rhs.m_numStacks) {
    }


    void SphereModifier::Apply()
    {
        auto ToCarthesian = [](float theta, float phi )
        {
            // return Vector3f(sinTheta * cosf(phi), sinTheta * sinf(phi), cosTheta);
            const auto sinPhi = sinf(phi);
            const auto sinTheta = sinf(theta);
            const auto cosPhi = cosf(phi);
            const auto cosTheta = cosf(theta);

            return Vector3f(sinTheta * cosPhi, cosTheta, sinTheta * sinPhi);
        };
        
        
        
        const auto thetaInc = ToRadians(180.0f) / m_numStacks;
        const auto phiInc   = ToRadians(360.0) / m_numSides;
        const auto dimsHalfs = m_dimensions * 0.5f;

        auto* pObject = GetGeometry();



        for (int i = 0; i < m_numSides; ++i)
        {
            const auto curPhi = phiInc * i;
            const auto nextPhi = curPhi + phiInc;
            
            for (int j = 0; j < m_numStacks; ++j) {

                const auto curTheta = j * thetaInc;
                const auto nextTheta = curTheta + thetaInc;

                const auto a = ToCarthesian(nextTheta, curPhi) * dimsHalfs;
                const auto b = ToCarthesian(curTheta, curPhi) * dimsHalfs;;
                const auto c = ToCarthesian(curTheta, nextPhi) * dimsHalfs;
                const auto d = ToCarthesian(nextTheta, nextPhi) * dimsHalfs;

                const auto uv1 = Vector2f(1.0f -  curPhi  / ToRadians(360),1.0f -  nextTheta / ToRadians(180.0f) );
                const auto uv2 = Vector2f(1.0f -  curPhi  / ToRadians(360),1.0f -  curTheta  / ToRadians(180.0f) );
                const auto uv3 = Vector2f(1.0f -  nextPhi / ToRadians(360),1.0f -  curTheta  / ToRadians(180.0f) );
                const auto uv4 = Vector2f(1.0f -  nextPhi / ToRadians(360),1.0f -  nextTheta / ToRadians(180.0f) );

                sUnprocessedFace face;
                face.m_verts.push_back(a);
                face.m_verts.push_back(b);
                face.m_verts.push_back(c);
                face.m_verts.push_back(d);

                face.m_uvs.push_back(uv1);
                face.m_uvs.push_back(uv2);
                face.m_uvs.push_back(uv3);
                face.m_uvs.push_back(uv4);
               // face.Flip();

                pObject->m_unprocessed.push_back(face);


                
            }
        }
        Triangulate(*pObject);

    }

    ObjPropVector SphereModifier::GetProperties()
    {
		if (!ContainsProperty(GetTypeNameStatic()))
		{
			auto pProperty = std::make_unique<PropertyData>();
			pProperty->m_name = GetTypeNameStatic();
			pProperty->AddProperty(
                { Properties<int>(&m_numSides, this, true, SIDES_MIN, SD_MAX), "Sides" }
			);
			pProperty->AddProperty(
                { Properties<int>(&m_numStacks, this, true, 2, SD_MAX), "Stacks" }
			);
			ObjectBase::AddPropertyData(std::move(pProperty));
		}

		auto retVal = ModifierBase::GetProperties();
		retVal.push_back(ObjectBase::GetPropertyData(GetTypeNameStatic()));
        return retVal;       
    }

    SphereModifier* SphereModifier::Clone() const
    {
        auto newMod = new  SphereModifier(*this);
        return newMod;
    }



    /// <summary>
    /// ConeModifier Implementation
    /// </summary>
    /// <param name="_pContext"></param>
    ConeModifier::ConeModifier(Context* _pContext)
        : ModifierBase(_pContext)
    {
        m_name = "Create Cone";
        m_numSides = 16;
        m_numStacks = 4;
        
    }

    void ConeModifier::Apply()
    {
       
        const auto sideInc = ToRadians(360.0f) / m_numSides;

        const auto startHeight = m_dimensions.y * -0.5f;
        const auto endHeight = m_dimensions.y * 0.5f;
        const auto top = Vector3f(0.0f, endHeight, 0.0f);

        auto* pObject = GetGeometry();


        for (int side = 0; side < m_numSides; ++side)
        {
            const auto curSide = Vector3f(cosf(side * sideInc) * m_dimensions.x * 0.5f,
                                         startHeight,
                                          sinf(side * sideInc) * m_dimensions.z * 0.5f);
            const auto nextSide = Vector3f(cosf(side * sideInc + sideInc) * m_dimensions.x * 0.5f,
                                           startHeight,
                                           sinf(side * sideInc + sideInc) * m_dimensions.z * 0.5f);

            const auto lengthAbInc = Length(top - curSide) / m_numStacks;
            const auto lengthCdINc = Length(top - nextSide) / m_numStacks;;

            const auto dirAb = Normalize(top - curSide);
            const auto dirCd = Normalize(top - nextSide);

            for (int stack = 0; stack < m_numStacks; ++stack)
            {
                const auto a =  curSide + (dirAb * (stack * lengthAbInc + lengthAbInc));
                const auto b =  curSide + (dirAb * (stack * lengthAbInc));
                const auto c =  nextSide + (dirCd * (stack * lengthCdINc));
                const auto d =  nextSide + (dirCd * (stack * lengthCdINc + lengthCdINc));

                sUnprocessedFace face;
                face.m_verts.emplace_back(a.x, a.y, a.z);
                face.m_verts.emplace_back(b.x, b.y, b.z);
                face.m_verts.emplace_back(c.x, c.y, c.z);
                face.m_verts.emplace_back(d.x, d.y, d.z);

                const auto aHeight = (startHeight - a.y) / ( startHeight - endHeight);
                const auto bHeight = (startHeight - b.y) / ( startHeight - endHeight);

                face.m_uvs.emplace_back(1.0f - (side * sideInc) / ToRadians(360.0f), aHeight);
                face.m_uvs.emplace_back(1.0f - (side * sideInc) / ToRadians(360.0f), bHeight);
                face.m_uvs.emplace_back(1.0f - (side * sideInc + sideInc) / ToRadians(360.0f), bHeight);
                face.m_uvs.emplace_back(1.0f - (side * sideInc + sideInc) / ToRadians(360.0f), aHeight);
                face.Flip();
                pObject->m_unprocessed.push_back(face);
            }
            //caps
            if (m_endCaps) {
            
                Vector3f capCur  = Vector3f(curSide.x, 0.0f, curSide.z);
                Vector3f capNext = Vector3f(nextSide.x, 0.0f, nextSide.z);

                const auto lengthCur = Length(capCur);
                const auto lengthNext = Length(capNext);

                const auto curDir = -1.0f * capCur / lengthCur;
                const auto nextDir = -1.0f * capNext / lengthNext;

                const auto curLengthInc = lengthCur / m_capStacks;
                const auto nextLengthInc = lengthNext / m_capStacks;

                auto NormalizedCoordinate = [](auto _input, auto _min, auto _max)
                {
                    const auto range = _max - _min;
                    return (_input - _min) / range;
                };

                const Vector3f start = m_dimensions * -0.5f;
                const Vector3f end = m_dimensions * 0.5f;
                const float sideInc = ToRadians(360.0f) / m_numSides;

                for (int stack = 0; stack < m_capStacks; ++stack)
                {
                    const Vector3f a = curSide + curDir * (curLengthInc * stack + curLengthInc);
                    const Vector3f b = curSide + curDir * (curLengthInc * stack);
                    const Vector3f c = nextSide + nextDir * (nextLengthInc * stack);
                    const Vector3f d = nextSide + nextDir * (nextLengthInc * stack + nextLengthInc);

                    sUnprocessedFace cap;
                    cap.m_verts.emplace_back(a.x, startHeight, a.z);
                    cap.m_verts.emplace_back(b.x, startHeight, b.z);
                    cap.m_verts.emplace_back(c.x, startHeight, c.z);
                    cap.m_verts.emplace_back(d.x, startHeight, d.z);

                    const auto uvStackCur = NormalizedCoordinate(a, start, end);
                    const auto uvSideCur = NormalizedCoordinate(b, start, end);
                    const auto uvSideNext = NormalizedCoordinate(c, start, end);
                    const auto uvStackNext = NormalizedCoordinate(d, start, end);

                    cap.m_uvs.emplace_back(uvStackCur.x, 1.0f - uvStackCur.z);
                    cap.m_uvs.emplace_back(uvSideCur.x, 1.0f - uvSideCur.z);
                    cap.m_uvs.emplace_back(uvSideNext.x, 1.0f - uvSideNext.z);
                    cap.m_uvs.emplace_back(uvStackNext.x, 1.0f - uvStackNext.z);
                    pObject->m_unprocessed.push_back(cap);
                 
                }
            }

        }

        Triangulate(*pObject);
    }

    ObjPropVector ConeModifier::GetProperties()
    {
		if (!ContainsProperty(GetTypeNameStatic()))
		{
			auto pProperty = std::make_unique<PropertyData>();
			pProperty->m_name = GetTypeNameStatic();
			pProperty->AddProperty(
				{ Properties<int>(&m_numSides, this,  true,SIDES_MIN, SD_MAX), "Sides" }
			);
			pProperty->AddProperty(
				{ Properties<int>(&m_numStacks, this, true, 2, SD_MAX), "Stacks" }
			);
			ObjectBase::AddPropertyData( std::move(pProperty));
		}

		auto retVal = ModifierBase::GetProperties();
		retVal.push_back(ObjectBase::GetPropertyData(GetTypeNameStatic()));
		return retVal;
    }

    ConeModifier* ConeModifier::Clone() const
    {
        auto newMod = new ConeModifier(*this);
        return newMod;
    }

    ConeModifier::ConeModifier(const ConeModifier& _rhs) : ModifierBase(_rhs)
        , m_numSides(_rhs.m_numSides)
        , m_numStacks(_rhs.m_numStacks)
        , m_capStacks(_rhs.m_capStacks)
        , m_endCaps(_rhs.m_endCaps) {}


    /// <summary>
    /// CylinderModifier Implementation
    /// </summary>
    /// <param name="_pContext"></param>
    CylinderModifier::CylinderModifier(Context* _pContext)
        : ModifierBase(_pContext)
    {
        m_name = "Create Cylinder";      
    }

    void CylinderModifier::Apply()
    {
        const auto stackInc = m_dimensions.y / m_numStacks;
        const auto sideInc = ToRadians(360.0f) / m_numSides;

        const auto startHeight = m_dimensions.y * -0.5f;
        const auto endHeight   = m_dimensions.y * 0.5f;
        const auto dir         = Vector3f(0.0f, 1.0f, 0.0f);

        auto* pObject = GetGeometry();


        for (int side = 0; side < m_numSides; ++side)
        {
            const auto curSide  = Vector3f( cosf( side * sideInc ) * m_dimensions.x * 0.5f,
                                            0.0f,
                                            sinf( side * sideInc ) * m_dimensions.z * 0.5f );
            const auto nextSide = Vector3f( cosf( side * sideInc + sideInc ) * m_dimensions.x * 0.5f,
                                            0.0f,
                                            sinf( side * sideInc + sideInc ) * m_dimensions.z * 0.5f );

            for (int stack = 0; stack < m_numStacks; ++stack)
            {
                const auto a = Vector3f( 0, startHeight, 0) + curSide + (dir *  (stack * stackInc + stackInc));
                const auto b = Vector3f( 0, startHeight, 0) + curSide + (dir *  (stack * stackInc));
                const auto c = Vector3f( 0, startHeight, 0) + nextSide + (dir * (stack * stackInc));
                const auto d = Vector3f( 0, startHeight, 0) + nextSide + (dir * (stack * stackInc + stackInc));

                sUnprocessedFace face;
                face.m_verts.emplace_back(a.x, a.y, a.z);
                face.m_verts.emplace_back(b.x, b.y, b.z);
                face.m_verts.emplace_back(c.x, c.y, c.z);
                face.m_verts.emplace_back(d.x, d.y, d.z);

                const auto aHeight = (stack * stackInc + stackInc) / (endHeight - startHeight);
                const auto bHeight = (stack * stackInc)  / (endHeight - startHeight);

                face.m_uvs.emplace_back(1.0f - (side * sideInc) / ToRadians(360.0f), aHeight);
                face.m_uvs.emplace_back(1.0f - (side * sideInc) / ToRadians(360.0f), bHeight);
                face.m_uvs.emplace_back(1.0f - (side * sideInc + sideInc) / ToRadians(360.0f), bHeight);
                face.m_uvs.emplace_back(1.0f - (side * sideInc + sideInc) / ToRadians(360.0f), aHeight);
                face.Flip();
                pObject->m_unprocessed.push_back(face);
            }
            //caps
            if (m_endCaps) {
                //const auto sideCur  = Vector2f{ xCur,  zCur };
               // const auto sideNext = Vector2f{ xNext, zNext };

                const auto lengthCur  = Length(curSide);
                const auto lengthNext = Length(nextSide);

                const auto curDir = -1.0f * curSide / lengthCur;
                const auto nextDir = -1.0f * nextSide / lengthNext;

                const auto curLengthInc = lengthCur / m_capStacks;
                const auto nextLengthInc = lengthNext / m_capStacks;

                auto NormalizedCoordinate = [](auto _input, auto _min, auto _max)
                {
                    const auto range = _max - _min;
                    return (_input - _min) / range;
                };

                const Vector3f start = m_dimensions * -0.5f;
                const Vector3f end = m_dimensions * 0.5f;
                const float sideInc  = ToRadians(360.0f) / m_numSides;

                for (int stack = 0; stack < m_capStacks; ++stack)
                {
                    const Vector3f a = curSide + curDir *   (curLengthInc * stack + curLengthInc);
                    const Vector3f b = curSide + curDir *   (curLengthInc * stack);
                    const Vector3f c = nextSide + nextDir * (nextLengthInc * stack);
                    const Vector3f d = nextSide + nextDir * (nextLengthInc * stack + nextLengthInc);

                    sUnprocessedFace topCap, bottomCap;
                    topCap.m_verts.emplace_back(a.x,startHeight, a.z);
                    topCap.m_verts.emplace_back(b.x,startHeight, b.z);
                    topCap.m_verts.emplace_back(c.x,startHeight, c.z);
                    topCap.m_verts.emplace_back(d.x,startHeight, d.z);

                    const auto uvStackCur  = NormalizedCoordinate(a, start, end);
                    const auto uvSideCur   = NormalizedCoordinate(b, start, end);
                    const auto uvSideNext  = NormalizedCoordinate(c, start, end);
                    const auto uvStackNext = NormalizedCoordinate(d, start, end);

                    topCap.m_uvs.emplace_back(uvStackCur.x,  1.0f - uvStackCur.z);
                    topCap.m_uvs.emplace_back(uvSideCur.x,   1.0f - uvSideCur.z);
                    topCap.m_uvs.emplace_back(uvSideNext.x,  1.0f - uvSideNext.z);
                    topCap.m_uvs.emplace_back(uvStackNext.x, 1.0f - uvStackNext.z);
                    bottomCap = topCap;

                    for (auto& v : bottomCap.m_verts)
                        v.y = endHeight;
                    bottomCap.Flip();
                   

                    pObject->m_unprocessed.push_back(topCap);
                    pObject->m_unprocessed.push_back(bottomCap);
                }
            }

        }

        Triangulate(*pObject);


    }

    ObjPropVector CylinderModifier::GetProperties()
    {
        
		if (!ContainsProperty(GetTypeNameStatic()))
		{
			auto pProperty = std::make_unique<PropertyData>();
			pProperty->m_name = GetTypeNameStatic();
            pProperty->AddProperty(
				{ Properties<int>(&m_numSides, this,  true,SIDES_MIN, SD_MAX), "Sides" }
			);
            pProperty->AddProperty(
				{ Properties<int>(&m_numStacks, this, true, STACKS_MIN, SD_MAX), "Stacks" }
			);

            pProperty->AddProperty(
				{ Properties<bool>(&m_endCaps, this), "End Caps" }
			);
            pProperty->AddProperty(
				{ Properties<int>(&m_capStacks, this, true, STACKS_MIN, SD_MAX), "End Caps Stacks" }
			);

			ObjectBase::AddPropertyData( std::move(pProperty));
		}

		auto retVal = ModifierBase::GetProperties();
		retVal.push_back(ObjectBase::GetPropertyData(GetTypeNameStatic()));
		return retVal;      
    
    }

    CylinderModifier* CylinderModifier::Clone() const
    {
        auto newMod = new CylinderModifier(*this);
        return newMod;
    }

    CylinderModifier::CylinderModifier(const CylinderModifier& _rhs) : ModifierBase(_rhs)
        , m_numSides(_rhs.m_numSides)
        , m_numStacks(_rhs.m_numStacks)
        , m_capStacks(_rhs.m_capStacks)
        , m_endCaps(_rhs.m_endCaps) {}

   

    

    MaterialModifier::MaterialModifier(Context* _pContext)
        : ModifierBase( _pContext )
    {

    }

    MaterialModifier::MaterialModifier(const MaterialModifier& _rhs) 
        : ModifierBase(_rhs)
        , m_pMaterial(_rhs.m_pMaterial) 
    {

    }


    void MaterialModifier::Apply()
    {
        GetGeometry()->m_pMaterial = m_pMaterial;
    }


    ObjPropVector MaterialModifier::GetProperties()
    {
        return ObjPropVector();
    }

    MaterialModifier* MaterialModifier::Clone() const
    {
        auto newMod = new MaterialModifier(*this);
        return newMod;
    }

    void MaterialModifier::SetMaterial(HWMaterial* _pMaterial) { 
        m_pMaterial = _pMaterial;         
    }

   
}

