#version 330 core
layout(triangles) in;
layout(line_strip, max_vertices = 6) out;

uniform float edgeBias = 0.001;

void EmitEdge(vec4 a, vec4 b)
{
    vec4 pa = a;
    vec4 pb = b;

    // move slightly closer to the camera
    pa.z -= edgeBias * pa.w;
    pb.z -= edgeBias * pb.w;

    gl_Position = pa;
    EmitVertex();

    gl_Position = pb;
    EmitVertex();

    EndPrimitive();
}

void main()
{
    EmitEdge(gl_in[0].gl_Position, gl_in[1].gl_Position);
    EmitEdge(gl_in[1].gl_Position, gl_in[2].gl_Position);
    EmitEdge(gl_in[2].gl_Position, gl_in[0].gl_Position);
}