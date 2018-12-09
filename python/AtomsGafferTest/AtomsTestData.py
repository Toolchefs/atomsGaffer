##########################################################################
#
#  Copyright (c) 2018, Toolchefs Ltd. All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are
#  met:
#
#      * Redistributions of source code must retain the above
#        copyright notice, this list of conditions and the following
#        disclaimer.
#
#      * Redistributions in binary form must reproduce the above
#        copyright notice, this list of conditions and the following
#        disclaimer in the documentation and/or other materials provided with
#        the distribution.
#
#      * Neither the name of John Haddon nor the names of
#        any other contributors to this software may be used to endorse or
#        promote products derived from this software without specific prior
#        written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
#  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
#  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
#  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
#  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
#  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
#  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
#  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
#  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
#  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
#  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
##########################################################################

import imath

import IECore
import IECoreScene

def buildTestPoints():
    positions = [ imath.V3f( 1, 0, 0 ), imath.V3f( 1, 0, 1 ), imath.V3f( 0, 0, 1 ), imath.V3f( 0, 0, 0 ) ]
    agent_id = [0, 1, 2, 3]
    agent_type = [ "atomsRobot", "atomsRobot", "atoms2Robot", "atoms2Robot" ]
    agent_variation = [ "Robot1", "Robot2", "RedRobot", "PurpleRobot" ]
    agent_lod = [ "", "B", "A", "" ]
    agent_velocity = [ imath.V3f( 1, 0, 0 ), imath.V3f( 1, 0, 0 ), imath.V3f( 0, 0, 0 ), imath.V3f( 1, 0, 0 ) ]
    agent_direction = [ imath.V3f( 1, 0, 0 ), imath.V3f( 1, 0, 0 ), imath.V3f( 1, 0, 0 ), imath.V3f( 1, 0, 0 ) ]
    agent_scale = [ imath.V3f( 1, 1, 1 ), imath.V3f( 1, 1, 1 ), imath.V3f( 1, 1, 1 ), imath.V3f( 1, 1, 1 ) ]
    agent_orientation = [ imath.Quatf( ), imath.Quatf( ), imath.Quatf( ), imath.Quatf( ) ]
    points = IECoreScene.PointsPrimitive( IECore.V3fVectorData( positions ) )

    points["atoms:agentId"] = IECoreScene.PrimitiveVariable( IECoreScene.PrimitiveVariable.Interpolation.Vertex,
                                                             IECore.IntVectorData( agent_id ))

    points["atoms:agentType"] = IECoreScene.PrimitiveVariable( IECoreScene.PrimitiveVariable.Interpolation.Vertex,
                                                               IECore.StringVectorData( agent_type ) )
    points["atoms:variation"] = IECoreScene.PrimitiveVariable( IECoreScene.PrimitiveVariable.Interpolation.Vertex,
                                                               IECore.StringVectorData( agent_variation ) )
    points["atoms:lod"] = IECoreScene.PrimitiveVariable( IECoreScene.PrimitiveVariable.Interpolation.Vertex,
                                                         IECore.StringVectorData( agent_lod ) )
    points["atoms:velocity"] = IECoreScene.PrimitiveVariable( IECoreScene.PrimitiveVariable.Interpolation.Vertex,
                                                              IECore.V3fVectorData( agent_velocity ) )
    points["atoms:direction"] = IECoreScene.PrimitiveVariable( IECoreScene.PrimitiveVariable.Interpolation.Vertex,
                                                               IECore.V3fVectorData( agent_direction ) )
    points["atoms:scale"] = IECoreScene.PrimitiveVariable( IECoreScene.PrimitiveVariable.Interpolation.Vertex,
                                                           IECore.V3fVectorData( agent_scale ) )
    points["atoms:orientation"] = IECoreScene.PrimitiveVariable( IECoreScene.PrimitiveVariable.Interpolation.Vertex,
                                                                 IECore.QuatfVectorData( agent_orientation ) )
    return points

def buildTestAttributes():
    attributes_map = {
        "0": {
            "agentType": IECore.StringData( "atomsRobot" ),
            "boundingBox": IECore.Box3dData( imath.Box3d( imath.V3d( -1.0 ), imath.V3d( 1.0 ) ) ),
            "metadata": {
                "testData": IECore.IntData( 2 )
            },
            "hash": IECore.UInt64Data( 0 ),
            "rootMatrix": IECore.M44dData( imath.M44d().translate( imath.V3d( 0.0, 0.0, 0.0 ) ) ),
            "poseNormalWorldMatrices": IECore.M44dVectorData( [ imath.M44d() ] ),
            "poseWorldMatrices": IECore.M44dVectorData( [ imath.M44d() ] ),
        },
        "1": {
            "agentType": IECore.StringData( "atomsRobot" ),
            "boundingBox": IECore.Box3dData( imath.Box3d( imath.V3d( -1.0 ), imath.V3d( 1.0 ) ) ),
            "metadata": {
                "testData": IECore.IntData( 2 )
            },
            "hash": IECore.UInt64Data( 0 ),
            "rootMatrix": IECore.M44dData( imath.M44d().translate( imath.V3d( 1.0, 0.0, 0.0 ) ) ),
            "poseNormalWorldMatrices": IECore.M44dVectorData( [ imath.M44d() ] ),
            "poseWorldMatrices": IECore.M44dVectorData( [ imath.M44d() ] ),
        },
        "2": {
            "agentType": IECore.StringData( "atomsRobot" ),
            "boundingBox": IECore.Box3dData( imath.Box3d( imath.V3d( -1.0 ), imath.V3d( 1.0 ) ) ),
            "metadata": {
                "testData": IECore.IntData( 2 )
            },
            "hash": IECore.UInt64Data( 0 ),
            "rootMatrix": IECore.M44dData( imath.M44d().translate( imath.V3d( 2.0, 0.0, 0.0 ) ) ),
            "poseNormalWorldMatrices": IECore.M44dVectorData( [ imath.M44d() ]),
            "poseWorldMatrices": IECore.M44dVectorData( [ imath.M44d() ]),
        },
        "3": {
            "agentType": IECore.StringData( "atomsRobot" ),
            "boundingBox": IECore.Box3dData( imath.Box3d( imath.V3d( -1.0 ), imath.V3d( 1.0 ) ) ),
            "metadata": {
                "testData": IECore.IntData( 2 )
            },
            "hash": IECore.UInt64Data( 0 ),
            "rootMatrix": IECore.M44dData( imath.M44d().translate( imath.V3d( 3.0, 0.0, 0.0 ) ) ),
            "poseNormalWorldMatrices": IECore.M44dVectorData( [ imath.M44d() ]),
            "poseWorldMatrices": IECore.M44dVectorData( [ imath.M44d() ]),
        },
        "frameOffset": IECore.FloatData( 0 ),
    }

    return { "atoms:agents": IECore.BlindDataHolder( attributes_map ) }

def buildCrowdTest():
    points = buildTestPoints()
    attributes_map = buildTestAttributes()
    return  IECore.CompoundObject( {
            "bound" : IECore.Box3fData( imath.Box3f( imath.V3f( 0, 0, 0 ), imath.V3f( 1, 0, 1 ) ) ),
            "attributes" : IECore.CompoundObject( ),
            "children" : {
                "crowd" : {
                    "bound" : IECore.Box3fData( points.bound() ),
                    "transform" : IECore.M44fData( imath.M44f() ),
                    "object" : points,
                    "attributes" : IECore.CompoundObject( attributes_map )
                },
            },
        }, )

def buildVariationTest():
    mesh = IECoreScene.SpherePrimitive()
    mesh = IECoreScene.MeshPrimitive.createBox( imath.Box3f( imath.V3f( -0.5 ), imath.V3f( 0.5 ) ) )

    verticesPerFace = IECore.IntVectorData( [4, 4, 4, 4, 4, 4] )
    vertexIds = IECore.IntVectorData( [ 0, 3, 2, 1, 2, 3, 7, 6, 0, 4, 7, 3, 1, 2, 6, 5, 4, 5, 6, 7, 0, 1, 5, 4 ] )
    p = IECore.V3fVectorData( [imath.V3f( 0, 0, 0 ), imath.V3f( 1, 0, 0 ), imath.V3f( 1, 0, 1 ), imath.V3f( 0, 0, 1 ),
                               imath.V3f( 0, 1, 0 ), imath.V3f( 1, 1, 0 ), imath.V3f( 1, 1, 1 ), imath.V3f( 0, 1, 1 ) ] )

    n = IECore.V3fVectorData( [imath.V3f( 0, -1, 0 ), imath.V3f( 0, -1, 0 ), imath.V3f( 0, -1, 0 ), imath.V3f( 0, -1, 0 ),
                               imath.V3f( 0, 0, 1 ), imath.V3f( 0, 0, 1 ), imath.V3f( 0, 0, 1 ), imath.V3f( 0, 0, 1 ),
                               imath.V3f( -1, 0, 0 ), imath.V3f( -1, 0, 0 ), imath.V3f( -1, 0, 0 ), imath.V3f( -1, 0, 0 ),
                               imath.V3f( 1, 0, 0 ), imath.V3f( 1, 0, 0 ), imath.V3f( 1, 0, 0 ), imath.V3f( 1, 0, 0 ),
                               imath.V3f( 0, 1, 0 ), imath.V3f( 0, 1, 0 ), imath.V3f( 0, 1, 0 ), imath.V3f( 0, 1, 0 ),
                               imath.V3f( 0, 0, -1 ), imath.V3f( 0, 0, -1 ), imath.V3f( 0, 0, -1 ), imath.V3f( 0, 0, -1 ),
                               ] )

    mesh2 = IECoreScene.MeshPrimitive( verticesPerFace, vertexIds, "linear", p )
    mesh2["N"] = IECoreScene.PrimitiveVariable( IECoreScene.PrimitiveVariable.Interpolation.FaceVarying, n )

    mesh_bbox = imath.Box3f( imath.V3f( 0.0 ), imath.V3f( 1.0 ) )
    joint_indices_count = IECore.IntVectorData( [ 1, 1, 1, 1, 1, 1, 1, 1 ] )
    joint_indices = IECore.IntVectorData( [ 0, 0, 0, 0, 0, 0, 0, 0 ] )
    joint_weights = IECore.FloatVectorData( [ 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0 ] )

    attributes_map = {

        "jointIndexCount" : joint_indices_count,
        "jointIndices" : joint_indices,
        "jointWeights" : joint_weights,

    }

    variations = {
        "attributes" : IECore.CompoundObject(),
        "children" : {
            "atoms2Robot" : {
                "attributes" : IECore.CompoundObject(),
                "children" : {
                    "PurpleRobot":
                    {
                        "attributes" : IECore.CompoundObject(),
                        "children" : {
                            "Body" :
                            {
                                "attributes" : IECore.CompoundObject(),
                                "children": {
                                    "RobotBody": {
                                        "object" : mesh,
                                        "attributes" : IECore.CompoundObject(  )
                                    },
                                }
                            }
                        },
                    },
                }
            },
        }
    }
    return variations

