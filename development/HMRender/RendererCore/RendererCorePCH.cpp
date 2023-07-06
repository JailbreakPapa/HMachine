#include <RendererCore/RendererCorePCH.h>

WD_STATICLINK_LIBRARY(RendererCore)
{
  if (bReturn)
    return;

  WD_STATICLINK_REFERENCE(RendererCore_AnimationSystem_AnimGraph_AnimNodes_BlackboardAnimNodes);
  WD_STATICLINK_REFERENCE(RendererCore_AnimationSystem_AnimGraph_AnimNodes_BoneWeightsAnimNode);
  WD_STATICLINK_REFERENCE(RendererCore_AnimationSystem_AnimGraph_AnimNodes_CombinePosesAnimNode);
  WD_STATICLINK_REFERENCE(RendererCore_AnimationSystem_AnimGraph_AnimNodes_ControllerInputAnimNode);
  WD_STATICLINK_REFERENCE(RendererCore_AnimationSystem_AnimGraph_AnimNodes_DebugAnimNodes);
  WD_STATICLINK_REFERENCE(RendererCore_AnimationSystem_AnimGraph_AnimNodes_EventAnimNode);
  WD_STATICLINK_REFERENCE(RendererCore_AnimationSystem_AnimGraph_AnimNodes_LocalToModelPoseAnimNode);
  WD_STATICLINK_REFERENCE(RendererCore_AnimationSystem_AnimGraph_AnimNodes_LogicAnimNodes);
  WD_STATICLINK_REFERENCE(RendererCore_AnimationSystem_AnimGraph_AnimNodes_MathAnimNodes);
  WD_STATICLINK_REFERENCE(RendererCore_AnimationSystem_AnimGraph_AnimNodes_MixClips1DAnimNode);
  WD_STATICLINK_REFERENCE(RendererCore_AnimationSystem_AnimGraph_AnimNodes_MixClips2DAnimNode);
  WD_STATICLINK_REFERENCE(RendererCore_AnimationSystem_AnimGraph_AnimNodes_ModelPoseOutputAnimNode);
  WD_STATICLINK_REFERENCE(RendererCore_AnimationSystem_AnimGraph_AnimNodes_PlayClipAnimNode);
  WD_STATICLINK_REFERENCE(RendererCore_AnimationSystem_AnimGraph_AnimNodes_PlaySequenceAnimNode);
  WD_STATICLINK_REFERENCE(RendererCore_AnimationSystem_AnimGraph_Implementation_AnimGraph);
  WD_STATICLINK_REFERENCE(RendererCore_AnimationSystem_AnimGraph_Implementation_AnimGraphNode);
  WD_STATICLINK_REFERENCE(RendererCore_AnimationSystem_AnimGraph_Implementation_AnimGraphPins);
  WD_STATICLINK_REFERENCE(RendererCore_AnimationSystem_AnimGraph_Implementation_AnimGraphResource);
  WD_STATICLINK_REFERENCE(RendererCore_AnimationSystem_Implementation_AnimPoseGenerator);
  WD_STATICLINK_REFERENCE(RendererCore_AnimationSystem_Implementation_AnimationClipResource);
  WD_STATICLINK_REFERENCE(RendererCore_AnimationSystem_Implementation_AnimationPose);
  WD_STATICLINK_REFERENCE(RendererCore_AnimationSystem_Implementation_EditableSkeleton);
  WD_STATICLINK_REFERENCE(RendererCore_AnimationSystem_Implementation_OzzUtils);
  WD_STATICLINK_REFERENCE(RendererCore_AnimationSystem_Implementation_Skeleton);
  WD_STATICLINK_REFERENCE(RendererCore_AnimationSystem_Implementation_SkeletonBuilder);
  WD_STATICLINK_REFERENCE(RendererCore_AnimationSystem_Implementation_SkeletonComponent);
  WD_STATICLINK_REFERENCE(RendererCore_AnimationSystem_Implementation_SkeletonPoseComponent);
  WD_STATICLINK_REFERENCE(RendererCore_AnimationSystem_Implementation_SkeletonResource);
  WD_STATICLINK_REFERENCE(RendererCore_BakedProbes_Implementation_BakedProbesComponent);
  WD_STATICLINK_REFERENCE(RendererCore_BakedProbes_Implementation_BakedProbesVolumeComponent);
  WD_STATICLINK_REFERENCE(RendererCore_BakedProbes_Implementation_BakedProbesWorldModule);
  WD_STATICLINK_REFERENCE(RendererCore_BakedProbes_Implementation_BakingInterface);
  WD_STATICLINK_REFERENCE(RendererCore_BakedProbes_Implementation_BakingUtils);
  WD_STATICLINK_REFERENCE(RendererCore_BakedProbes_Implementation_ProbeTreeSectorResource);
  WD_STATICLINK_REFERENCE(RendererCore_Components_Implementation_AlwaysVisibleComponent);
  WD_STATICLINK_REFERENCE(RendererCore_Components_Implementation_BeamComponent);
  WD_STATICLINK_REFERENCE(RendererCore_Components_Implementation_CameraComponent);
  WD_STATICLINK_REFERENCE(RendererCore_Components_Implementation_FogComponent);
  WD_STATICLINK_REFERENCE(RendererCore_Components_Implementation_OccluderComponent);
  WD_STATICLINK_REFERENCE(RendererCore_Components_Implementation_RenderComponent);
  WD_STATICLINK_REFERENCE(RendererCore_Components_Implementation_RenderTargetActivatorComponent);
  WD_STATICLINK_REFERENCE(RendererCore_Components_Implementation_RopeRenderComponent);
  WD_STATICLINK_REFERENCE(RendererCore_Components_Implementation_SkyBoxComponent);
  WD_STATICLINK_REFERENCE(RendererCore_Components_Implementation_SpriteComponent);
  WD_STATICLINK_REFERENCE(RendererCore_Components_Implementation_SpriteRenderer);
  WD_STATICLINK_REFERENCE(RendererCore_Debug_Implementation_DebugRenderer);
  WD_STATICLINK_REFERENCE(RendererCore_Debug_Implementation_DebugTextComponent);
  WD_STATICLINK_REFERENCE(RendererCore_Debug_Implementation_Inconsolata);
  WD_STATICLINK_REFERENCE(RendererCore_Debug_Implementation_SimpleASCIIFont);
  WD_STATICLINK_REFERENCE(RendererCore_Decals_Implementation_DecalAtlasResource);
  WD_STATICLINK_REFERENCE(RendererCore_Decals_Implementation_DecalComponent);
  WD_STATICLINK_REFERENCE(RendererCore_Decals_Implementation_DecalResource);
  WD_STATICLINK_REFERENCE(RendererCore_GPUResourcePool_Implementation_GPUResourcePool);
  WD_STATICLINK_REFERENCE(RendererCore_Lights_Implementation_AmbientLightComponent);
  WD_STATICLINK_REFERENCE(RendererCore_Lights_Implementation_BoxReflectionProbeComponent);
  WD_STATICLINK_REFERENCE(RendererCore_Lights_Implementation_ClusteredDataExtractor);
  WD_STATICLINK_REFERENCE(RendererCore_Lights_Implementation_ClusteredDataProvider);
  WD_STATICLINK_REFERENCE(RendererCore_Lights_Implementation_DirectionalLightComponent);
  WD_STATICLINK_REFERENCE(RendererCore_Lights_Implementation_LightComponent);
  WD_STATICLINK_REFERENCE(RendererCore_Lights_Implementation_PointLightComponent);
  WD_STATICLINK_REFERENCE(RendererCore_Lights_Implementation_ReflectionPool);
  WD_STATICLINK_REFERENCE(RendererCore_Lights_Implementation_ReflectionPoolData);
  WD_STATICLINK_REFERENCE(RendererCore_Lights_Implementation_ReflectionProbeComponentBase);
  WD_STATICLINK_REFERENCE(RendererCore_Lights_Implementation_ReflectionProbeData);
  WD_STATICLINK_REFERENCE(RendererCore_Lights_Implementation_ReflectionProbeMapping);
  WD_STATICLINK_REFERENCE(RendererCore_Lights_Implementation_ReflectionProbeUpdater);
  WD_STATICLINK_REFERENCE(RendererCore_Lights_Implementation_ShadowPool);
  WD_STATICLINK_REFERENCE(RendererCore_Lights_Implementation_SimplifiedDataExtractor);
  WD_STATICLINK_REFERENCE(RendererCore_Lights_Implementation_SimplifiedDataProvider);
  WD_STATICLINK_REFERENCE(RendererCore_Lights_Implementation_SkyLightComponent);
  WD_STATICLINK_REFERENCE(RendererCore_Lights_Implementation_SphereReflectionProbeComponent);
  WD_STATICLINK_REFERENCE(RendererCore_Lights_Implementation_SpotLightComponent);
  WD_STATICLINK_REFERENCE(RendererCore_Material_Implementation_MaterialResource);
  WD_STATICLINK_REFERENCE(RendererCore_Meshes_Implementation_CpuMeshResource);
  WD_STATICLINK_REFERENCE(RendererCore_Meshes_Implementation_CustomMeshComponent);
  WD_STATICLINK_REFERENCE(RendererCore_Meshes_Implementation_DynamicMeshBufferResource);
  WD_STATICLINK_REFERENCE(RendererCore_Meshes_Implementation_InstancedMeshComponent);
  WD_STATICLINK_REFERENCE(RendererCore_Meshes_Implementation_MeshBufferResource);
  WD_STATICLINK_REFERENCE(RendererCore_Meshes_Implementation_MeshBufferUtils);
  WD_STATICLINK_REFERENCE(RendererCore_Meshes_Implementation_MeshComponent);
  WD_STATICLINK_REFERENCE(RendererCore_Meshes_Implementation_MeshComponentBase);
  WD_STATICLINK_REFERENCE(RendererCore_Meshes_Implementation_MeshRenderer);
  WD_STATICLINK_REFERENCE(RendererCore_Meshes_Implementation_MeshResource);
  WD_STATICLINK_REFERENCE(RendererCore_Meshes_Implementation_MeshResourceDescriptor);
  WD_STATICLINK_REFERENCE(RendererCore_Meshes_Implementation_SkinnedMeshComponent);
  WD_STATICLINK_REFERENCE(RendererCore_Meshes_Implementation_SkinnedMeshRenderer);
  WD_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_ExtractedRenderData);
  WD_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_Extractor);
  WD_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_FrameDataProvider);
  WD_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_InstanceDataProvider);
  WD_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_Passes_AOPass);
  WD_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_Passes_AntialiasingPass);
  WD_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_Passes_BloomPass);
  WD_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_Passes_BlurPass);
  WD_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_Passes_CopyTexturePass);
  WD_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_Passes_DepthOnlyPass);
  WD_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_Passes_ForwardRenderPass);
  WD_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_Passes_LSAOPass);
  WD_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_Passes_MsaaResolvePass);
  WD_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_Passes_MsaaUpscalePass);
  WD_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_Passes_OpaqueForwardRenderPass);
  WD_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_Passes_ReflectionFilterPass);
  WD_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_Passes_SelectionHighlightPass);
  WD_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_Passes_SeparatedBilateralBlur);
  WD_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_Passes_SimpleRenderPass);
  WD_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_Passes_SkyRenderPass);
  WD_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_Passes_SourcePass);
  WD_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_Passes_StereoTestPass);
  WD_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_Passes_TargetPass);
  WD_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_Passes_TonemapPass);
  WD_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_Passes_TransparentForwardRenderPass);
  WD_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_RenderData);
  WD_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_RenderPipeline);
  WD_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_RenderPipelineNode);
  WD_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_RenderPipelinePass);
  WD_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_RenderPipelineResource);
  WD_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_RenderPipelineResourceLoader);
  WD_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_SortingFunctions);
  WD_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_View);
  WD_STATICLINK_REFERENCE(RendererCore_Pipeline_Implementation_ViewRenderMode);
  WD_STATICLINK_REFERENCE(RendererCore_Rasterizer_Implementation_RasterizerObject);
  WD_STATICLINK_REFERENCE(RendererCore_Rasterizer_Implementation_RasterizerView);
  WD_STATICLINK_REFERENCE(RendererCore_Rasterizer_Thirdparty_Occluder);
  WD_STATICLINK_REFERENCE(RendererCore_Rasterizer_Thirdparty_Rasterizer);
  WD_STATICLINK_REFERENCE(RendererCore_RenderContext_Implementation_RenderContext);
  WD_STATICLINK_REFERENCE(RendererCore_RenderWorld_Implementation_RenderWorld);
  WD_STATICLINK_REFERENCE(RendererCore_ShaderCompiler_Implementation_PermutationGenerator);
  WD_STATICLINK_REFERENCE(RendererCore_ShaderCompiler_Implementation_ShaderCompiler);
  WD_STATICLINK_REFERENCE(RendererCore_ShaderCompiler_Implementation_ShaderManager);
  WD_STATICLINK_REFERENCE(RendererCore_ShaderCompiler_Implementation_ShaderParser);
  WD_STATICLINK_REFERENCE(RendererCore_Shader_Implementation_ConstantBufferStorage);
  WD_STATICLINK_REFERENCE(RendererCore_Shader_Implementation_Helper);
  WD_STATICLINK_REFERENCE(RendererCore_Shader_Implementation_ShaderPermutationBinary);
  WD_STATICLINK_REFERENCE(RendererCore_Shader_Implementation_ShaderPermutationResource);
  WD_STATICLINK_REFERENCE(RendererCore_Shader_Implementation_ShaderResource);
  WD_STATICLINK_REFERENCE(RendererCore_Shader_Implementation_ShaderStageBinary);
  WD_STATICLINK_REFERENCE(RendererCore_Shader_Implementation_ShaderStateDescriptor);
  WD_STATICLINK_REFERENCE(RendererCore_Textures_Texture2DResource);
  WD_STATICLINK_REFERENCE(RendererCore_Textures_Texture3DResource);
  WD_STATICLINK_REFERENCE(RendererCore_Textures_TextureCubeResource);
  WD_STATICLINK_REFERENCE(RendererCore_Textures_TextureLoader);
  WD_STATICLINK_REFERENCE(RendererCore_Textures_TextureUtils);
  WD_STATICLINK_REFERENCE(RendererCore_Utils_Implementation_WorldGeoExtractionUtil);
}