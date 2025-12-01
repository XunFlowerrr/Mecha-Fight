#version 330 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;

in VS_OUT
{
    vec3 FragPosVS;
    vec3 NormalVS;
} fs_in;

void main()
{
    gPosition = fs_in.FragPosVS;
    gNormal = normalize(fs_in.NormalVS);
}

