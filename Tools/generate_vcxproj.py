#!/usr/bin/env python3
"""Generate PathTracer.vcxproj / .filters / PathTracing.slnx from the source tree."""
import os, sys, uuid, pathlib

ROOT = pathlib.Path(sys.argv[1])          # repository root
EXTRA_PROJECTS = sys.argv[2:]             # extra project paths for the .slnx
PROJ = ROOT / 'PathTracer'
GUID = '{6C3B2F2A-5D1E-4C6B-9E2A-3F1D7A9B8C01}'

def rel(p):
    return str(p.relative_to(PROJ)).replace('/', '\\')

cpps, headers, nones = [], [], []
for p in sorted(PROJ.rglob('*')):
    if not p.is_file():
        continue
    r = p.relative_to(PROJ)
    if r.parts[0] in ('bin', 'obj', 'x64'):
        continue
    if p.suffix == '.cpp':
        cpps.append(p)
    elif p.suffix == '.h':
        headers.append(p)
    elif p.suffix in ('.hlsl', '.hlsli', '.txt'):
        nones.append(p)

def pch_for(p):
    d = p.parent.relative_to(PROJ)
    up = '..\\' * len(d.parts)
    return up + 'Common\\stdafx.h'

configs = [('Debug', True), ('Release', False)]
out = []
w = out.append
w('<?xml version="1.0" encoding="utf-8"?>')
w('<Project DefaultTargets="Build" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">')
w('  <ItemGroup Label="ProjectConfigurations">')
for c, _ in configs:
    w(f'    <ProjectConfiguration Include="{c}|x64"><Configuration>{c}</Configuration><Platform>x64</Platform></ProjectConfiguration>')
w('  </ItemGroup>')
w('  <PropertyGroup Label="Globals">')
w('    <VCProjectVersion>18.0</VCProjectVersion>')
w('    <Keyword>Win32Proj</Keyword>')
w(f'    <ProjectGuid>{GUID}</ProjectGuid>')
w('    <RootNamespace>PathTracer</RootNamespace>')
w('    <WindowsTargetPlatformVersion>10.0</WindowsTargetPlatformVersion>')
w('  </PropertyGroup>')
w('  <Import Project="$(VCTargetsPath)\\Microsoft.Cpp.Default.props" />')
for c, dbg in configs:
    w(f'  <PropertyGroup Condition="\'$(Configuration)|$(Platform)\'==\'{c}|x64\'" Label="Configuration">')
    w('    <ConfigurationType>Application</ConfigurationType>')
    w(f'    <UseDebugLibraries>{"true" if dbg else "false"}</UseDebugLibraries>')
    w('    <PlatformToolset>v145</PlatformToolset>')
    if not dbg:
        w('    <WholeProgramOptimization>true</WholeProgramOptimization>')
    w('    <CharacterSet>Unicode</CharacterSet>')
    w('  </PropertyGroup>')
w('  <Import Project="$(VCTargetsPath)\\Microsoft.Cpp.props" />')
w('  <PropertyGroup>')
w('    <OutDir>$(ProjectDir)bin\\$(Platform)\\$(Configuration)\\</OutDir>')
w('    <IntDir>$(ProjectDir)obj\\$(Platform)\\$(Configuration)\\</IntDir>')
w('    <TargetName>PathTracer</TargetName>')
w('  </PropertyGroup>')
for c, dbg in configs:
    w(f'  <ItemDefinitionGroup Condition="\'$(Configuration)|$(Platform)\'==\'{c}|x64\'">')
    w('    <ClCompile>')
    w('      <WarningLevel>Level3</WarningLevel>')
    w('      <SDLCheck>true</SDLCheck>')
    w(f'      <PreprocessorDefinitions>{"_DEBUG" if dbg else "NDEBUG"};_CONSOLE;%(PreprocessorDefinitions)</PreprocessorDefinitions>')
    w('      <ConformanceMode>true</ConformanceMode>')
    w('      <LanguageStandard>stdcpp20</LanguageStandard>')
    if not dbg:
        w('      <FunctionLevelLinking>true</FunctionLevelLinking>')
        w('      <IntrinsicFunctions>true</IntrinsicFunctions>')
    w('      <PrecompiledHeader>Use</PrecompiledHeader>')
    w('      <PrecompiledHeaderOutputFile>$(IntDir)$(ProjectName).pch</PrecompiledHeaderOutputFile>')
    w('      <AdditionalIncludeDirectories>$(ProjectDir)ThirdParty\\ImGui;%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>')
    w('      <AdditionalOptions>/utf-8 %(AdditionalOptions)</AdditionalOptions>')
    w('    </ClCompile>')
    w('    <Link>')
    w('      <SubSystem>Console</SubSystem>')
    w('      <GenerateDebugInformation>true</GenerateDebugInformation>')
    w('      <AdditionalDependencies>d3d12.lib;dxgi.lib;d3dcompiler.lib;dwmapi.lib;imm32.lib;comdlg32.lib;%(AdditionalDependencies)</AdditionalDependencies>')
    w('    </Link>')
    w('  </ItemDefinitionGroup>')
w('  <ItemGroup>')
for p in cpps:
    r = rel(p)
    if r.startswith('ThirdParty'):
        w(f'    <ClCompile Include="{r}"><PrecompiledHeader>NotUsing</PrecompiledHeader></ClCompile>')
    elif r == 'Common\\stdafx.cpp':
        w(f'    <ClCompile Include="{r}"><PrecompiledHeader>Create</PrecompiledHeader><PrecompiledHeaderFile>stdafx.h</PrecompiledHeaderFile></ClCompile>')
    else:
        w(f'    <ClCompile Include="{r}"><PrecompiledHeaderFile>{pch_for(p)}</PrecompiledHeaderFile></ClCompile>')
w('  </ItemGroup>')
w('  <ItemGroup>')
for p in headers:
    w(f'    <ClInclude Include="{rel(p)}" />')
w('  </ItemGroup>')
w('  <ItemGroup>')
for p in nones:
    w(f'    <None Include="{rel(p)}" />')
w('  </ItemGroup>')
w('  <Import Project="$(VCTargetsPath)\\Microsoft.Cpp.targets" />')
w('  <ItemGroup>')
w('    <RuntimeShader Include="$(ProjectDir)Renderer\\Shader\\**\\*.hlsl;$(ProjectDir)Renderer\\Shader\\**\\*.hlsli" />')
w('    <RuntimeScene Include="$(ProjectDir)..\\Scenes\\**\\*.json" />')
w('    <RuntimeAsset Include="$(ProjectDir)..\\Asset\\**\\*.*" />')
w('  </ItemGroup>')
w('  <Target Name="CopyRuntimeResources" AfterTargets="Build">')
w('    <Copy SourceFiles="@(RuntimeShader)" DestinationFiles="@(RuntimeShader->\'$(OutDir)Shaders\\%(RecursiveDir)%(Filename)%(Extension)\')" SkipUnchangedFiles="true" />')
w('    <Copy SourceFiles="@(RuntimeScene)" DestinationFiles="@(RuntimeScene->\'$(OutDir)Scenes\\%(RecursiveDir)%(Filename)%(Extension)\')" SkipUnchangedFiles="true" />')
w('    <Copy SourceFiles="@(RuntimeAsset)" DestinationFiles="@(RuntimeAsset->\'$(OutDir)Asset\\%(RecursiveDir)%(Filename)%(Extension)\')" SkipUnchangedFiles="true" />')
w('  </Target>')
w('</Project>')
(PROJ / 'PathTracer.vcxproj').write_text('﻿' + '\r\n'.join(out) + '\r\n', encoding='utf-8')

# Filters: one filter per folder.
folders = set()
items = []
for kind, group in (('ClCompile', cpps), ('ClInclude', headers), ('None', nones)):
    for p in group:
        r = rel(p)
        folder = '\\'.join(r.split('\\')[:-1])
        parts = folder.split('\\') if folder else []
        for i in range(1, len(parts) + 1):
            folders.add('\\'.join(parts[:i]))
        items.append((kind, r, folder))
f = []
f.append('<?xml version="1.0" encoding="utf-8"?>')
f.append('<Project ToolsVersion="4.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">')
f.append('  <ItemGroup>')
for folder in sorted(folders):
    f.append(f'    <Filter Include="{folder}"><UniqueIdentifier>{{{uuid.uuid5(uuid.NAMESPACE_URL, folder)}}}</UniqueIdentifier></Filter>')
f.append('  </ItemGroup>')
f.append('  <ItemGroup>')
for kind, r, folder in items:
    if folder:
        f.append(f'    <{kind} Include="{r}"><Filter>{folder}</Filter></{kind}>')
    else:
        f.append(f'    <{kind} Include="{r}" />')
f.append('  </ItemGroup>')
f.append('</Project>')
(PROJ / 'PathTracer.vcxproj.filters').write_text('﻿' + '\r\n'.join(f) + '\r\n', encoding='utf-8')

s = ['<Solution>', '  <Configurations>', '    <Platform Name="x64" />', '  </Configurations>',
     '  <Project Path="PathTracer/PathTracer.vcxproj" />']
for e in EXTRA_PROJECTS:
    s.append(f'  <Project Path="{e}" />')
s.append('</Solution>')
(ROOT / 'PathTracing.slnx').write_text('\r\n'.join(s) + '\r\n', encoding='utf-8')
print(len(cpps), 'cpp', len(headers), 'h', len(nones), 'none')
