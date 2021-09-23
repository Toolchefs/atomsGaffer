//////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2018, Toolchefs Ltd. All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are
//  met:
//
//      * Redistributions of source code must retain the above
//        copyright notice, this list of conditions and the following
//        disclaimer.
//
//      * Redistributions in binary form must reproduce the above
//        copyright notice, this list of conditions and the following
//        disclaimer in the documentation and/or other materials provided with
//        the distribution.
//
//      * Neither the name of John Haddon nor the names of
//        any other contributors to this software may be used to endorse or
//        promote products derived from this software without specific prior
//        written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
//  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
//  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
//  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
//  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
//  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
//  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
//  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//////////////////////////////////////////////////////////////////////////

#ifndef ATOMSGAFFER_ATOMSMATHTRNASLATOR_H
#define ATOMSGAFFER_ATOMSMATHTRNASLATOR_H

#include "AtomsCore/AtomsMath.h"

#include "ImathVec.h"
#include "ImathQuat.h"
#include "ImathBox.h"
#include "ImathMatrix.h"

template<typename T, typename Y>
inline void convertFromAtoms( T& out, const Y& in )
{
    out = in;
}

template<>
inline void convertFromAtoms( Imath::V2d& out, const AtomsCore::Vector2& in )
{
    out.x = in.x;
    out.y = in.y;
}

template<>
inline void convertFromAtoms( Imath::V2f& out, const AtomsCore::Vector2f& in )
{
    out.x = in.x;
    out.y = in.y;
}

template<>
inline void convertFromAtoms( Imath::V2f& out, const AtomsCore::Vector2& in )
{
    out.x = static_cast<float>( in.x );
    out.y = static_cast<float>( in.y );
}

template<>
inline void convertFromAtoms( Imath::V3d& out, const AtomsCore::Vector3& in )
{
    out.x = in.x;
    out.y = in.y;
    out.z = in.z;
}

template<>
inline void convertFromAtoms( Imath::V3f& out, const AtomsCore::Vector3f& in )
{
    out.x = in.x;
    out.y = in.y;
    out.z = in.z;
}

template<>
inline void convertFromAtoms( Imath::V3f& out, const AtomsCore::Vector3& in )
{
    out.x = static_cast<float>( in.x );
    out.y = static_cast<float>( in.y );
    out.z = static_cast<float>( in.z );
}

template<>
inline void convertFromAtoms( Imath::Quatd& out, const AtomsCore::Quaternion& in )
{
    convertFromAtoms(out.v, in.v);
    out.r = in.r;
}

template<>
inline void convertFromAtoms( Imath::Quatf& out, const AtomsCore::Quaternionf& in )
{
    convertFromAtoms(out.v, in.v);
    out.r = in.r;
}

template<>
inline void convertFromAtoms( Imath::Quatf& out, const AtomsCore::Quaternion& in )
{
    convertFromAtoms(out.v, in.v);
    out.r = static_cast<float>( in.r );
}

template<>
inline void convertFromAtoms( Imath::M44d& out, const AtomsCore::Matrix& in )
{
    for ( unsigned short i = 0; i < 4; ++i )
    {
        for ( unsigned short j = 0; j < 4; ++j )
        {
            out[i][j] = in[i][j];
        }
    }
}

template<>
inline void convertFromAtoms( Imath::M44f& out, const AtomsCore::Matrixf& in )
{
    for ( unsigned short i = 0; i < 4; ++i )
    {
        for ( unsigned short j = 0; j < 4; ++j )
        {
            out[i][j] = in[i][j];
        }
    }
}

template<>
inline void convertFromAtoms( Imath::M44f& out, const AtomsCore::Matrix& in )
{
    for ( unsigned short i = 0; i < 4; ++i )
    {
        for ( unsigned short j = 0; j < 4; ++j )
        {
            out[i][j] = static_cast<float>( in[i][j] );
        }
    }
}

template<>
inline void convertFromAtoms( Imath::Box3d& out, const AtomsCore::Box3& in )
{
    convertFromAtoms(out.min, in.min);
    convertFromAtoms(out.max, in.max);
}

template<>
inline void convertFromAtoms( Imath::Box3f& out, const AtomsCore::Box3f& in )
{
    convertFromAtoms(out.min, in.min);
    convertFromAtoms(out.max, in.max);
}

template<>
inline void convertFromAtoms( Imath::Box3f& out, const AtomsCore::Box3& in )
{
    convertFromAtoms(out.min, in.min);
    convertFromAtoms(out.max, in.max);
}

///////////
// vectors
///////////

template<typename T, typename Y>
inline void convertFromAtoms( std::vector<T>& out, const std::vector<Y>& in )
{
    size_t startIndex = out.size();
    out.resize( out.size() + in.size() );
    for ( size_t i = 0; i < in.size(); ++i)
    {
        convertFromAtoms( out[startIndex + i], in[i] );
    }
}


#endif //ATOMSGAFFER_ATOMSMATHTRNASLATOR_H
