#version 410

layout(location = 0) out vec4 fragColor;

uniform mat4 um4mv;
uniform mat4 um4p;

in VertexData
{
    vec3 N; // eye space normal
    vec3 L; // eye space light vector
    vec3 H; // eye space halfway vector
    vec2 texcoord;
} vertexData;

uniform sampler2D tex;
uniform samplerCube skybox;
uniform int state;
uniform int isTextured;

in vec3 tttt;
in vec3 normal;
void main()
{	
	if(state == 0){
		if( isTextured == 1){
			vec3 texColor = texture(tex,vertexData.texcoord).rgb;
			fragColor = vec4(texColor, 1.0);
		}else{
			fragColor = vec4(normal, 1.0);
		}
	}else if(state == 1){
		fragColor = texture(skybox, tttt);
	}
}