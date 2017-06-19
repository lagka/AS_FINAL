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

struct MaterialParameters {
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	float shininess;
};

struct LightSourceParameters {
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	vec4 position;
	vec4 halfVector;
	vec4 spotDirection;
	float spotExponent;
	float spotCutoff; // (range: [0.0,90.0], 180.0)
	float spotCosCutoff; // (range: [1.0,0.0],-1.0)
	float constantAttenuation;
	float linearAttenuation;
	float quadraticAttenuation;
};

uniform sampler2D tex;
uniform samplerCube skybox;
uniform int state;
uniform int isTextured;
uniform MaterialParameters Material;
uniform LightSourceParameters LightSource[4];
in vec3 tttt;
in vec3 normal;

float fatt , dis;
float fatt2  = 0, dis2 = 0 ;

float spotlight_effect = 1;

vec4 vv4ambient_D = vec4(0,0,0,0);
vec4 Diffuse = vec4(0,0,0,0);
vec4 Specular = vec4(0,0,0,0);
vec4 Diffuse_point = vec4(0,0,0,0);
vec4 Specular_point = vec4(0,0,0,0);
vec4 Diffuse_point2 = vec4(0,0,0,0);
vec4 Specular_point2 = vec4(0,0,0,0);
vec4  N , L ,V , H ;
vec4 vv4color;
vec4 camera_pos = vec4(0 , 200 , 15  , 1);

void main()
{	
	N = normalize(vec4(normal , 0) );
	L = normalize(LightSource[1].position -  vec4(tttt , 0));
	V = normalize(camera_pos - vec4(tttt , 0));
	H = normalize(L + V) ;
	vv4ambient_D = LightSource[0].ambient * Material.ambient ;
	Diffuse =   LightSource[1].diffuse * Material.diffuse* max(dot(N,L) , 0.0) ;
	Specular = LightSource[1].specular*Material.specular*pow(max( dot(N,H) , 0.0),50) ;
	
	vv4color = 	vv4ambient_D + Diffuse +Specular  ; 

	if(state == 0){
		if( isTextured == 1){
			vec3 texColor = texture(tex,vertexData.texcoord).rgb;
			fragColor = vec4(texColor, 1.0) * Diffuse;
		}else{
			fragColor = vv4color ;
			//fragColor = vec4(normal, 1.0);
		}
	}else if(state == 1){
		fragColor = texture(skybox, tttt);
	}
}