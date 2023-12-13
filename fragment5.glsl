#version 330 core

//--- out_Color: 버텍스 세이더에서 입력받는 색상 값
//--- FragColor: 출력할 색상의 값으로 프레임 버퍼로 전달 됨.

in vec4 out_Color; //--- 버텍스 세이더에게서 전달 받음
in vec3 FragPos;
in vec3 Normal;
in vec3 TexCoord;

out vec4 FragColor; //--- 색상 출력

uniform vec3 cameraPos;
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform sampler2D outTexture;

void main(void)
{
	
	vec3 ambientLight = vec3(0.7f, 0.7f, 0.7f);
	//vec3 ambientLight = vec3(1.0f, 1.0f, 1.0f);
	vec3 ambient = ambientLight * lightColor;

	vec3 NV = normalize(Normal);
	vec3 lightDir = normalize(lightPos - FragPos);
	float diffuseLight = max(dot(NV, lightDir), 0.0);
	vec3 diffuse = diffuseLight * lightColor;

	int shininess = 128;
	vec3 viewDir = normalize(cameraPos - FragPos);
	vec3 reflectDir = reflect(lightDir, NV);
	float specularLight = max(dot(viewDir, reflectDir), 0.0);
	specularLight = pow(specularLight, shininess);
	vec3 specular = specularLight * lightColor;

	vec4 result = vec4((diffuse + specular + ambient),1.0) * out_Color;
	FragColor = vec4(result);
	vec2 tex = vec2(TexCoord.x, TexCoord.y);
	FragColor = texture(outTexture, tex) * FragColor;
}