/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once


#include "BlendSpaceNode.h"
#include <AzCore/std/containers/vector.h>

namespace EMotionFX
{
    class AnimGraphPose;
    class AnimGraphInstance;
    class BlendSpaceParamEvaluator;


    class EMFX_API BlendSpace2DNode
        : public BlendSpaceNode
    {
    public:
        AZ_RTTI(BlendSpace2DNode, "{5C0DADA2-FE74-468F-A755-55AEBE579C45}", BlendSpaceNode)
        AZ_CLASS_ALLOCATOR_DECL

        enum
        {
            INPUTPORT_XVALUE = 0,
            INPUTPORT_YVALUE = 1,
            INPUTPORT_INPLACE = 2,
            OUTPUTPORT_POSE = 0
        };

        enum
        {
            PORTID_INPUT_XVALUE = 0,
            PORTID_INPUT_YVALUE = 1,
            PORTID_INPUT_INPLACE = 2,
            PORTID_OUTPUT_POSE = 0
        };

        struct Triangle
        {
            Triangle(uint16_t indexA, uint16_t indexB, uint16_t indexC);

            uint16_t m_vertIndices[3];
        };
        using Triangles = AZStd::vector<Triangle>;

        struct Edge
        {
            uint16_t m_vertIndices[2];

            bool operator==(const Edge& rhs) const
            {
                return (m_vertIndices[0] == rhs.m_vertIndices[0]) &&
                       (m_vertIndices[1] == rhs.m_vertIndices[1]);
            }
        };
        using Edges = AZStd::vector<Edge>;
        struct EdgeHasher
        {
            size_t operator()(const Edge& edge) const
            {
                // Hash based on the idea that there won't typically be more than 10000 verts.
                // No serious harm even if there happen to be more.
                return 10000 * (AZ::u32)edge.m_vertIndices[0] + edge.m_vertIndices[1];
            }
        };

        struct CurrentTriangleInfo
        {
            AZ::u32 m_triangleIndex;
            float   m_weights[3];
        };
        struct CurrentEdgeInfo
        {
            AZ::u32 m_edgeIndex;
            float   m_u; // parameter for the closest point along the edge
        };

        class EMFX_API UniqueData
            : public AnimGraphNodeData
        {
            EMFX_ANIMGRAPHOBJECTDATA_IMPLEMENT_LOADSAVE
        public:
            AZ_CLASS_ALLOCATOR_DECL

            UniqueData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance);
            ~UniqueData();

            AZ::Vector2 ConvertToNormalizedSpace(const AZ::Vector2& pt) const;

            void Reset() override;

        public:
            MotionInfos                     m_motionInfos;
            bool                            m_allMotionsHaveSyncTracks;
            AZStd::vector<AZ::Vector2>      m_motionCoordinates;
            AZStd::vector<AZ::Vector2>      m_normMotionPositions; // Normalized motion positions
            AZ::Vector2                     m_rangeMin; // min of x & y range
            AZ::Vector2                     m_rangeMax; // max of x & y range
            AZ::Vector2                     m_rangeCenter;
            AZ::Vector2                     m_normalizationScale;
            Triangles                       m_triangles; // Delaunay triangles tessellating the parameter space
            Edges                           m_outerEdges; // outer (i.e., boundary) edges of the triangulated region

            AZ::Vector2                     m_currentPosition;
            AZ::Vector2                     m_normCurrentPosition; // normalized current point
            CurrentTriangleInfo             m_currentTriangle; // Info about the triangle in which the current point lies.
            CurrentEdgeInfo                 m_currentEdge; // When the point is not inside any triangle, information
            // about the closest point on the outer edge
            BlendInfos                      m_blendInfos;
            AZ::u32                         m_masterMotionIdx; // index of the master motion for syncing

            bool                            m_hasDegenerateTriangles; // to notify the UI
        };

        BlendSpace2DNode();
        ~BlendSpace2DNode();

        void Reinit() override;
        bool InitAfterLoading(AnimGraph* animGraph) override;

        bool GetValidCalculationMethodsAndEvaluators() const;
        const char* GetAxisLabel(int axisIndex) const;

        // AnimGraphNode overrides
        bool    GetSupportsVisualization() const override { return true; }
        bool    GetSupportsDisable() const override { return true; }
        bool    GetHasVisualGraph() const override { return true; }
        bool    GetHasOutputPose() const override { return true; }
        bool    GetNeedsNetTimeSync() const override { return true; }
        AZ::Color GetVisualColor() const override { return AZ::Color(0.23f, 0.71f, 0.78f, 1.0f); }
        void    OnUpdateUniqueData(AnimGraphInstance* animGraphInstance) override;
        AnimGraphPose* GetMainOutputPose(AnimGraphInstance* animGraphInstance) const override { return GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue(); }

        // AnimGraphObject overrides
        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        //! Update the locations of motions in the blend space.
        void UpdateMotionPositions(UniqueData& uniqueData);

        //! Called to set the current position from GUI.
        void SetCurrentPosition(const AZ::Vector2& point);

        void SetSyncMasterMotionId(const AZStd::string& syncMasterMotionId);
        const AZStd::string& GetSyncMasterMotionId() const;

        void SetEvaluatorTypeX(const AZ::TypeId& evaluatorType);
        const AZ::TypeId& GetEvaluatorTypeX() const;
        BlendSpaceParamEvaluator* GetEvaluatorX() const;

        void SetCalculationMethodX(ECalculationMethod calculationMethod);
        ECalculationMethod GetCalculationMethodX() const;

        void SetEvaluatorTypeY(const AZ::TypeId& evaluatorType);
        const AZ::TypeId& GetEvaluatorTypeY() const;
        BlendSpaceParamEvaluator* GetEvaluatorY() const;

        void SetCalculationMethodY(ECalculationMethod calculationMethod);
        ECalculationMethod GetCalculationMethodY() const;

        void SetSyncMode(ESyncMode syncMode);
        ESyncMode GetSyncMode() const;

        void SetEventFilterMode(EBlendSpaceEventMode eventFilterMode);
        EBlendSpaceEventMode GetEventFilterMode() const;

        // BlendSpaceNode overrides

        //! Compute the position of the motion in blend space.
        void ComputeMotionCoordinates(const AZStd::string& motionId, AnimGraphInstance* animGraphInstance, AZ::Vector2& position) override;

        //! Restore the motion coordinates that are set to automatic mode back to the computed values.
        void RestoreMotionCoordinates(BlendSpaceMotion& motion, AnimGraphInstance* animGraphInstance) override;

        void SetMotions(const AZStd::vector<BlendSpaceMotion>& motions) override;
        const AZStd::vector<BlendSpaceMotion>& GetMotions() const override;

        bool GetIsInPlace(AnimGraphInstance* animGraphInstance) const;

        static void Reflect(AZ::ReflectContext* context);

    protected:
        // AnimGraphNode overrides
        void Output(AnimGraphInstance* animGraphInstance) override;
        void TopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void Rewind(AnimGraphInstance* animGraphInstance) override;

    private:
        bool    UpdateMotionInfos(AnimGraphInstance* animGraphInstance);
        void    ComputeNormalizationInfo(UniqueData& uniqueData);
        void    UpdateTriangulation(UniqueData& uniqueData);
        void    DetermineOuterEdges(UniqueData& uniqueData);

        AZ::Vector2 GetCurrentSamplePosition(AnimGraphInstance* animGraphInstance, const UniqueData& uniqueData);
        void    UpdateBlendingInfoForCurrentPoint(UniqueData& uniqueData);
        bool    FindTriangleForCurrentPoint(UniqueData& uniqueData);
        bool    FindOuterEdgeClosestToCurrentPoint(UniqueData& uniqueData);
        void    SetBindPoseAtOutput(AnimGraphInstance* animGraphInstance);

    private:
        AZ::Crc32 GetEvaluatorXVisibility() const;
        AZ::Crc32 GetEvaluatorYVisibility() const;
        AZ::Crc32 GetSyncOptionsVisibility() const;

        AZStd::vector<BlendSpaceMotion> m_motions;
        AZStd::string                   m_syncMasterMotionId;
        BlendSpaceParamEvaluator*       m_evaluatorX;
        AZ::TypeId                      m_evaluatorTypeX;
        ECalculationMethod              m_calculationMethodX;
        BlendSpaceParamEvaluator*       m_evaluatorY;
        AZ::TypeId                      m_evaluatorTypeY;
        ECalculationMethod              m_calculationMethodY;
        ESyncMode                       m_syncMode;
        EBlendSpaceEventMode            m_eventFilterMode;

        AZ::Vector2 m_currentPositionSetInteractively;

        static const float s_epsilonForBarycentricCoords;
    };
}   // namespace EMotionFX
