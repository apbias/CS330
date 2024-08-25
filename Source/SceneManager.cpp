///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();

	// initialize the texture collection
	for (int i = 0; i < 16; i++)
	{
		m_textureIDs[i].tag = "/0";
		m_textureIDs[i].ID = -1;
	}
	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;

	// destroy the created OpenGL textures
	DestroyGLTextures();
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;
	glm::mat3 normal;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;
	normal = glm::transpose(glm::inverse(glm::mat3(modelView)));
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
		m_pShaderManager->setMat3Value("normal", normal);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

void SceneManager::DefineObjectMaterials()
{
	/*** STUDENTS - add the code BELOW for defining object materials. ***/
	/*** There is no limit to the number of object materials that can ***/
	/*** be defined. Refer to the code in the OpenGL Sample for help  ***/

	// Marble Material (Desk)
	OBJECT_MATERIAL marbleMaterial;
	marbleMaterial.diffuseColor = glm::vec3(0.8f, 0.8f, 0.8f); // Light grayish-white
	marbleMaterial.specularColor = glm::vec3(0.9f, 0.9f, 0.9f); // High reflectivity
	marbleMaterial.shininess = 50.0f; // Shiny surface
	marbleMaterial.tag = "marble";
	m_objectMaterials.push_back(marbleMaterial);

	// Paper Material (Magazine Stack)
	OBJECT_MATERIAL paperMaterial;
	paperMaterial.diffuseColor = glm::vec3(0.9f, 0.9f, 0.8f); // Off-white with slight warmth
	paperMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f); // Low reflectivity
	paperMaterial.shininess = 10.0f; // Slight sheen
	paperMaterial.tag = "paper";
	m_objectMaterials.push_back(paperMaterial);

	// Fabric Material (Mousepad)
	OBJECT_MATERIAL fabricMaterial;
	fabricMaterial.diffuseColor = glm::vec3(0.6f, 0.2f, 0.4f); // Color of the fabric (e.g., purple-pink)
	fabricMaterial.specularColor = glm::vec3(0.2f, 0.1f, 0.1f); // Low reflectivity
	fabricMaterial.shininess = 5.0f; // Matte surface
	fabricMaterial.tag = "fabric";
	m_objectMaterials.push_back(fabricMaterial);

	// Plastic Material (Keyboard and Mouse)
	OBJECT_MATERIAL plasticMaterial;
	plasticMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f); // Neutral gray
	plasticMaterial.specularColor = glm::vec3(0.3f, 0.3f, 0.3f); // Moderate reflectivity
	plasticMaterial.shininess = 30.0f; // Semi-shiny surface
	plasticMaterial.tag = "plastic";
	m_objectMaterials.push_back(plasticMaterial);

	// Ceramic Material (Flower Vase)
	OBJECT_MATERIAL ceramicMaterial;
	ceramicMaterial.diffuseColor = glm::vec3(0.9f, 0.9f, 0.9f); // Bright white
	ceramicMaterial.specularColor = glm::vec3(0.8f, 0.8f, 0.8f); // High reflectivity
	ceramicMaterial.shininess = 40.0f; // Shiny but not as reflective as marble
	ceramicMaterial.tag = "ceramic";
	m_objectMaterials.push_back(ceramicMaterial);

	// Screen Material (Monitor)
	OBJECT_MATERIAL screenMaterial;
	screenMaterial.diffuseColor = glm::vec3(0.1f, 0.1f, 0.1f);  // Dark gray, representing a powered-off screen or bezel.
	screenMaterial.specularColor = glm::vec3(0.9f, 0.9f, 0.9f); // High specular to give it a glossy appearance.
	screenMaterial.shininess = 50.0; // Higher shininess for a reflective, glass-like surface.
	screenMaterial.tag = "screen";
	m_objectMaterials.push_back(screenMaterial);

	// Wood Material 
	OBJECT_MATERIAL woodMaterial;
	woodMaterial.diffuseColor = glm::vec3(0.3f, 0.2f, 0.1f);
	woodMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	woodMaterial.shininess = 0.3;
	woodMaterial.tag = "wood";
	m_objectMaterials.push_back(woodMaterial);


}
/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// this line of code is NEEDED for telling the shaders to render 
	// the 3D scene with custom lighting, if no light sources have
	// been added then the display window will be black - to use the 
	// default OpenGL lighting then comment out the following line
	m_pShaderManager->setBoolValue(g_UseLightingName, true);


	/*** STUDENTS - add the code BELOW for setting up light sources ***/
	/*** Up to four light sources can be defined. Refer to the code ***/
	/*** in the OpenGL Sample for help                              ***/

	// Directional Light (adjusted to be cooler and brighter)
	m_pShaderManager->setVec3Value("directionalLight.direction", glm::normalize(glm::vec3(0.0f, -1.0f, 0.0f)));
	m_pShaderManager->setVec3Value("directionalLight.ambient", { 0.2f, 0.2f, 0.2f });   // Neutral ambient
	m_pShaderManager->setVec3Value("directionalLight.diffuse", { 0.8f, 0.8f, 0.8f });   // Bright white diffuse
	m_pShaderManager->setVec3Value("directionalLight.specular", { 0.5f, 0.5f, 0.5f });  // Neutral specular
	m_pShaderManager->setBoolValue("directionalLight.bActive", true);

	// Point Light 1 (adjusted to be cooler and brighter)
	m_pShaderManager->setVec3Value("pointLights[0].position", { 0.0f, 8.0f, 4.0f });    // Positioned above the scene
	m_pShaderManager->setVec3Value("pointLights[0].ambient", { 0.1f, 0.1f, 0.1f });     // Low, neutral ambient
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", { 0.8f, 0.8f, 0.9f });     // Bright, slightly cool diffuse
	m_pShaderManager->setVec3Value("pointLights[0].specular", { 0.8f, 0.8f, 0.9f });    // Matching specular
	m_pShaderManager->setFloatValue("pointLights[0].constant", 1.0f);					// No attenuation at 1.0
	m_pShaderManager->setFloatValue("pointLights[0].linear", 0.09f);					// Linear falloff for a more natural light (linear decrease in light intensity as distance increases)
	m_pShaderManager->setFloatValue("pointLights[0].quadratic", 0.032f);				// Quadratic falloff for a more natural light (more rapid decrease in light intensity as distance increases)
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);

	// New Point Light 2 (cool temperature to balance the scene)
	m_pShaderManager->setVec3Value("pointLights[1].position", { 4.0f, 6.0f, -4.0f });   // Positioned to the side
	m_pShaderManager->setVec3Value("pointLights[1].ambient", { 0.05f, 0.05f, 0.1f });   // Slight cool ambient
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", { 0.4f, 0.4f, 0.8f });     // Cool blue diffuse
	m_pShaderManager->setVec3Value("pointLights[1].specular", { 0.4f, 0.4f, 0.8f });    // Matching specular
	m_pShaderManager->setFloatValue("pointLights[1].constant", 1.0f);					// No attenuation at 1.0
	m_pShaderManager->setFloatValue("pointLights[1].linear", 0.09f);					// Linear falloff for a more natural light (linear decrease in light intensity as distance increases)
	m_pShaderManager->setFloatValue("pointLights[1].quadratic", 0.032f);				// Quadratic falloff for a more natural light (more rapid decrease in light intensity as distance increases)
	m_pShaderManager->setBoolValue("pointLights[1].bActive", true);


}


 /***********************************************************
  *  LoadSceneTextures()
  *
  *  This method is used for preparing the 3D scene by loading
  *  the shapes, textures in memory to support the 3D scene
  *  rendering
  ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	/*** STUDENTS - add the code BELOW for loading the textures that ***/
	/*** will be used for mapping to objects in the 3D scene. Up to  ***/
	/*** 16 textures can be loaded per scene. Refer to the code in   ***/
	/*** the OpenGL Sample for help.                                 ***/
	bool bReturn = false;

	bReturn = CreateGLTexture("textures/topdesk.jpg", "desk");
	bReturn = CreateGLTexture("textures/monitorfront.jpg", "monitorfront");
	bReturn = CreateGLTexture("textures/wood.jpg", "wood");
	bReturn = CreateGLTexture("textures/silver.jpg", "silver");
	bReturn = CreateGLTexture("textures/magfront.jpg", "magfront");
	bReturn = CreateGLTexture("textures/mousepad.jpg", "mousepad");
	bReturn = CreateGLTexture("textures/magsides.jpg", "magsides");
	bReturn = CreateGLTexture("textures/magcover.jpg", "magcover");
	bReturn = CreateGLTexture("textures/keyboard.jpg", "keyboard");
	bReturn = CreateGLTexture("textures/mousetop.jpg", "mousetop");
	bReturn = CreateGLTexture("textures/vaseblue.jpg", "vaseblue");
	bReturn = CreateGLTexture("textures/whitevase.jpg", "whitevase");



	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// define the materials for objects in the scene
	DefineObjectMaterials();
	// add and define the light sources for the scene
	SetupSceneLights();
	// load the textures for the 3D scene
	LoadSceneTextures();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadPrismMesh();
	m_basicMeshes->LoadSphereMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	/****************************************************************/
	/***Draw Plane Mesh for Desk                                   */
	/****************************************************************/

	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);


	// set the active material for the next draw command
	SetShaderMaterial("marble");
	// set the texture for the next draw command
	SetShaderTexture("desk");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	/****************************************************************/


/****************************************************************/
/***Draw Box Mesh for Keyboard                                  */
/****************************************************************/

// declare the variables for the transformations
	glm::vec3 scaleXYZ2;
	float XrotationDegrees2 = 0.0f;
	float YrotationDegrees2 = 0.0f;
	float ZrotationDegrees2 = 0.0f;
	glm::vec3 positionXYZ2;

	// set the XYZ scale for the mesh
	scaleXYZ2 = glm::vec3(8.0f, 0.5f, 4.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees2 = 0.0f;
	YrotationDegrees2 = 0.0f;
	ZrotationDegrees2 = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ2 = glm::vec3(0.0f, 0.25f, 6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ2,
		XrotationDegrees2,
		YrotationDegrees2,
		ZrotationDegrees2,
		positionXYZ2);

	SetShaderColor(0.8f, 0.8f, 0.78f, 1.0f);

	SetShaderMaterial("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	/***Draw Cylinder Mesh for Mouse Pad                            */
	/****************************************************************/

	// declare the variables for the transformations
	glm::vec3 scaleXYZ3;
	float XrotationDegrees3 = 0.0f;
	float YrotationDegrees3 = 0.0f;
	float ZrotationDegrees3 = 0.0f;
	glm::vec3 positionXYZ3;

	// set the XYZ scale for the mesh
	scaleXYZ3 = glm::vec3(3.0f, 0.25f, 3.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees3 = 0.0f;
	YrotationDegrees3 = 0.0f;
	ZrotationDegrees3 = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ3 = glm::vec3(9.0f, 0.0f, 6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ3,
		XrotationDegrees3,
		YrotationDegrees3,
		ZrotationDegrees3,
		positionXYZ3);

	SetShaderColor(0.91f, 0.67f, 0.75f, 1.0f);

	// set the texture for the next draw command
	SetShaderTexture("mousepad");
	// set the active material for the next draw command
	SetShaderMaterial("fabric");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/
	/***Draw Box Mesh Trackpad                                      */
	/****************************************************************/

	// declare the variables for the transformations
	glm::vec3 scaleXYZ20;
	float XrotationDegrees20 = 0.0f;
	float YrotationDegrees20 = 0.0f;
	float ZrotationDegrees20 = 0.0f;
	glm::vec3 positionXYZ20;

	// set the XYZ scale for the mesh
	scaleXYZ20 = glm::vec3(5.0f, 0.25f, 5.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees20 = 8.0f;
	YrotationDegrees20 = 0.0f;
	ZrotationDegrees20 = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ20 = glm::vec3(-7.0f, 0.25f, -3.0f);
	
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ20,
		XrotationDegrees20,
		YrotationDegrees20,
		ZrotationDegrees20,
		positionXYZ20);

	SetShaderColor(0.91f, 0.67f, 0.75f, 1.0f);

	// set the texture for the next draw command
	SetShaderTexture("silver");
	// set the active material for the next draw command
	SetShaderMaterial("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	/***Draw Cylinder Mesh for Trackpad Base                       */
	/****************************************************************/

	// declare the variables for the transformations
	glm::vec3 scaleXYZ21;
	float XrotationDegrees21 = 0.0f;
	float YrotationDegrees21 = 0.0f;
	float ZrotationDegrees21 = 0.0f;
	glm::vec3 positionXYZ21;

	// set the XYZ scale for the cylinder mesh
	scaleXYZ21 = glm::vec3(.5f, 5.0f, .5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees21 = 0.0f;
	YrotationDegrees21 = 0.0f;
	ZrotationDegrees21 = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ21 = glm::vec3(-4.5f, 0.15f, -5.2f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ21,
		XrotationDegrees21,
		YrotationDegrees21,
		ZrotationDegrees21,
		positionXYZ21);

	// set a slightly darker color for the cylinder
	SetShaderColor(0.85f, 0.62f, 0.70f, 1.0f);

	// set the texture for the next draw command
	SetShaderTexture("silver");

	// set the active material for the next draw command
	SetShaderMaterial("plastic");

	// draw the cylinder mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();


	/****************************************************************/
	/***Draw Box Mesh for Monitor   (stand bottom)                  */
	/****************************************************************/

	// declare the variables for the transformations
	glm::vec3 scaleXYZ4;
	float XrotationDegrees4 = 0.0f;
	float YrotationDegrees4 = 0.0f;
	float ZrotationDegrees4 = 0.0f;
	glm::vec3 positionXYZ4;

	// set the XYZ scale for the mesh
	scaleXYZ4 = glm::vec3(5.0f, 0.5f, 4.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees4 = 0.0f;
	YrotationDegrees4 = 0.0f;
	ZrotationDegrees4 = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ4 = glm::vec3(0.0f, 0.25f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ4,
		XrotationDegrees4,
		YrotationDegrees4,
		ZrotationDegrees4,
		positionXYZ4);

	SetShaderColor(0.88f, 0.88f, 0.88f, 1.0f);

	// set the texture for the next draw command
	SetShaderTexture("silver");
	// set the active material for the next draw command
	SetShaderMaterial("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();


	/****************************************************************/
	/***Draw Box Mesh for Monitor   (stand arm)                  */
	/****************************************************************/

	// declare the variables for the transformations
	glm::vec3 scaleXYZ5;
	float XrotationDegrees5 = 0.0f;
	float YrotationDegrees5 = 0.0f;
	float ZrotationDegrees5 = 0.0f;
	glm::vec3 positionXYZ5;

	// set the XYZ scale for the mesh
	scaleXYZ5 = glm::vec3(4.0f, 0.5f, 5.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees5 = -45.0f;
	YrotationDegrees5 = 0.0f;
	ZrotationDegrees5 = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ5 = glm::vec3(0.0f, 2.35f, -5.75f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ5,
		XrotationDegrees5,
		YrotationDegrees5,
		ZrotationDegrees5,
		positionXYZ5);

	SetShaderColor(0.08f, 0.08f, 0.88f, 1.0f);

	// set the texture for the next draw command
	SetShaderTexture("silver");
	// set the active material for the next draw command
	SetShaderMaterial("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	/***Draw Triangle Mesh for Monitor   (stand bottom)             */
	/****************************************************************/

	// declare the variables for the transformations
	glm::vec3 scaleXYZ6;
	float XrotationDegrees6 = 0.0f;
	float YrotationDegrees6 = 0.0f;
	float ZrotationDegrees6 = 0.0f;
	glm::vec3 positionXYZ6;

	// set the XYZ scale for the mesh
	scaleXYZ6 = glm::vec3(0.5f, 4.0f, 0.25f);

	// set the XYZ rotation for the mesh
	XrotationDegrees6 = -90.0f;
	YrotationDegrees6 = 90.0f;
	ZrotationDegrees6 = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ6 = glm::vec3(0.0f, 0.63f, -7.75f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ6,
		XrotationDegrees6,
		YrotationDegrees6,
		ZrotationDegrees6,
		positionXYZ6);

	SetShaderColor(0.88f, 0.08f, 0.08f, 1.0f);

	// set the texture for the next draw command
	SetShaderTexture("silver");
	// set the active material for the next draw command
	SetShaderMaterial("plastic");
	// draw the mesh with transformation values
	m_basicMeshes->DrawPrismMesh();

	/****************************************************************/
	/***Draw Box Mesh for Monitor   (monitor)                       */
	/****************************************************************/

	// declare the variables for the transformations
	glm::vec3 scaleXYZ7;
	float XrotationDegrees7 = 0.0f;
	float YrotationDegrees7 = 0.0f;
	float ZrotationDegrees7 = 0.0f;
	glm::vec3 positionXYZ7;

	// set the XYZ scale for the mesh
	scaleXYZ7 = glm::vec3(20.0f, 0.01f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees7 = 90.0f;
	YrotationDegrees7 = 0.0f;
	ZrotationDegrees7 = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ7 = glm::vec3(0.0f, 8.0f, -2.99f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ7,
		XrotationDegrees7,
		YrotationDegrees7,
		ZrotationDegrees7,
		positionXYZ7);

	SetShaderColor(1, 1, 1, 1);

	// set the texture for the next draw command
	SetShaderTexture("monitorfront");
	// set the active material for the next draw command
	SetShaderMaterial("screen");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	/***Draw Box Mesh for Monitor Edge   (monitor edge)             */
	/****************************************************************/
	
	// declare the variables for the transformations
	glm::vec3 scaleXYZ14;
	float XrotationDegrees14 = 0.0f;
	float YrotationDegrees14 = 0.0f;
	float ZrotationDegrees14 = 0.0f;
	glm::vec3 positionXYZ14;

	// set the XYZ scale for the mesh
	scaleXYZ14 = glm::vec3(20.1f, 1.0f, 10.1f);

	// set the XYZ rotation for the mesh
	XrotationDegrees14 = 90.0f;
	YrotationDegrees14 = 0.0f;
	ZrotationDegrees14 = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ14 = glm::vec3(0.0f, 8.0f, -3.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ14,
		XrotationDegrees14,
		YrotationDegrees14,
		ZrotationDegrees14,
		positionXYZ14);



	SetShaderColor(1, 1, 1, 1);

	// set the texture for the next draw command
	SetShaderTexture("silver");

	// set the active material for the next draw command
	SetShaderMaterial("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	/***Draw Half Sphere Mesh for Vase   (bottom)                   */
	/****************************************************************/

	// declare the variables for the transformations
	glm::vec3 scaleXYZ8;
	float XrotationDegrees8 = 0.0f;
	float YrotationDegrees8 = 0.0f;
	float ZrotationDegrees8 = 0.0f;
	glm::vec3 positionXYZ8;

	// set the XYZ scale for the mesh
	scaleXYZ8 = glm::vec3(2.0f, 2.0f, 2.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees8 = 0.0f;
	YrotationDegrees8 = 0.0f;
	ZrotationDegrees8 = 180.0f;

	// set the XYZ position for the mesh
	positionXYZ8 = glm::vec3(-15.0f, 1.0f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ8,
		XrotationDegrees8,
		YrotationDegrees8,
		ZrotationDegrees8,
		positionXYZ8);

	SetShaderColor(0.39f, 0.39f, 0.45f, 1.0f);

	// set the texture for the next draw command
	SetShaderTexture("whitevase");

	// set the active material for the next draw command
	SetShaderMaterial("ceramic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawHalfSphereMesh();

	/****************************************************************/
	/***Draw Cylinder Mesh for Vase   (middle)                      */
	/****************************************************************/

	// declare the variables for the transformations
	glm::vec3 scaleXYZ9;
	float XrotationDegrees9 = 0.0f;
	float YrotationDegrees9 = 0.0f;
	float ZrotationDegrees9 = 0.0f;
	glm::vec3 positionXYZ9;

	// set the XYZ scale for the mesh
	scaleXYZ9 = glm::vec3(2.0f, 4.76f, 2.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees9 = 0.0f;
	YrotationDegrees9 = 0.0f;
	ZrotationDegrees9 = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ9 = glm::vec3(-15.0f, 1.0f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ9,
		XrotationDegrees9,
		YrotationDegrees9,
		ZrotationDegrees9,
		positionXYZ9);

	SetShaderColor(0.88f, 0.88f, 0.88f, 1.0f);

	

	// set the texture for the next draw command
	SetShaderTexture("whitevase");

	// set the active material for the next draw command
	SetShaderMaterial("ceramic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/
	/***Draw Cylinder Mesh for Vase   (middle lines)                */
	/****************************************************************/

	// declare the variables for the transformations
	glm::vec3 scaleXYZ10;
	float XrotationDegrees10 = 0.0f;
	float YrotationDegrees10 = 0.0f;
	float ZrotationDegrees10 = 0.0f;
	glm::vec3 positionXYZ10;

	// set the XYZ scale for the mesh
	scaleXYZ10 = glm::vec3(2.05f, 1.0f, 2.05f);

	// set the XYZ rotation for the mesh
	XrotationDegrees10 = 0.0f;
	YrotationDegrees10 = 0.0f;
	ZrotationDegrees10 = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ10 = glm::vec3(-15.0f, 1.0f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ10,
		XrotationDegrees10,
		YrotationDegrees10,
		ZrotationDegrees10,
		positionXYZ10);

	SetShaderColor(0.0f, 0.0f, 0.5f, 1.0f);

	 

	// set the texture for the next draw command
	SetShaderTexture("vaseblue");

	// set the shader material for the next draw command
	SetShaderMaterial("ceramic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/
	/***Draw Cylinder Mesh for Vase   (middle lines)                */
	/****************************************************************/

	// declare the variables for the transformations
	glm::vec3 scaleXYZ11;
	float XrotationDegrees11 = 0.0f;
	float YrotationDegrees11 = 0.0f;
	float ZrotationDegrees11 = 0.0f;
	glm::vec3 positionXYZ11;

	// set the XYZ scale for the mesh
	scaleXYZ11 = glm::vec3(2.05f, 1.0f, 2.05f);

	// set the XYZ rotation for the mesh
	XrotationDegrees11 = 0.0f;
	YrotationDegrees11 = 0.0f;
	ZrotationDegrees11 = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ11 = glm::vec3(-15.0f, 2.25f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ11,
		XrotationDegrees11,
		YrotationDegrees11,
		ZrotationDegrees11,
		positionXYZ11);

	SetShaderColor(0.0f, 0.0f, 0.5f, 1.0f);

	// set the texture for the next draw command
	SetShaderTexture("ceramic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/
	/***Draw Cylinder Mesh for Vase   (middle lines)                */
	/****************************************************************/

	// declare the variables for the transformations
	glm::vec3 scaleXYZ12;
	float XrotationDegrees12 = 0.0f;
	float YrotationDegrees12 = 0.0f;
	float ZrotationDegrees12 = 0.0f;
	glm::vec3 positionXYZ12;

	// set the XYZ scale for the mesh
	scaleXYZ12 = glm::vec3(2.05f, 1.0f, 2.05f);

	// set the XYZ rotation for the mesh
	XrotationDegrees12 = 0.0f;
	YrotationDegrees12 = 0.0f;
	ZrotationDegrees12 = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ12 = glm::vec3(-15.0f, 3.50f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ12,
		XrotationDegrees12,
		YrotationDegrees12,
		ZrotationDegrees12,
		positionXYZ12);

	SetShaderColor(0.0f, 0.0f, 0.5f, 1.0f);

	// set the texture for the next draw command
	SetShaderTexture("ceramic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/
	/***Draw Cylinder Mesh for Vase   (middle lines)                */
	/****************************************************************/

	// declare the variables for the transformations
	glm::vec3 scaleXYZ13;
	float XrotationDegrees13 = 0.0f;
	float YrotationDegrees13 = 0.0f;
	float ZrotationDegrees13 = 0.0f;
	glm::vec3 positionXYZ13;

	// set the XYZ scale for the mesh
	scaleXYZ13 = glm::vec3(2.05f, 1.0f, 2.05f);

	// set the XYZ rotation for the mesh
	XrotationDegrees13 = 0.0f;
	YrotationDegrees13 = 0.0f;
	ZrotationDegrees13 = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ13 = glm::vec3(-15.0f, 4.75f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ13,
		XrotationDegrees13,
		YrotationDegrees13,
		ZrotationDegrees13,
		positionXYZ13);

	SetShaderColor(0.0f, 0.0f, 0.5f, 1.0f);

	// set the texture for the next draw command
	SetShaderTexture("ceramic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();


	/****************************************************************/
	/***Draw Box Mesh for Magazines   (all)                         */
	/****************************************************************/

	// declare the variables for the transformations
	glm::vec3 scaleXYZ15;
	float XrotationDegrees15 = 0.0f;
	float YrotationDegrees15 = 0.0f;
	float ZrotationDegrees15 = 0.0f;
	glm::vec3 positionXYZ15;

	// set the XYZ scale for the mesh
	scaleXYZ15 = glm::vec3(9.0f, 2.50f, 6.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees15 = 0.0f;
	YrotationDegrees15 = 0.0f;
	ZrotationDegrees15 = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ15 = glm::vec3(13.0f, 1.5f, -5.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ15,
		XrotationDegrees15,
		YrotationDegrees15,
		ZrotationDegrees15,
		positionXYZ15);

	SetShaderColor(0.5f, 0.5f, 0.5f, 1.0f);

	// set the texture for the next draw command
	SetShaderTexture("magsides");

	// set the active material for the next draw command
	SetShaderMaterial("paper");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	/***Draw Box Mesh for Magazines   (front)                       */
	/****************************************************************/

	// declare the variables for the transformations
	glm::vec3 scaleXYZ16;
	float XrotationDegrees16 = 0.0f;
	float YrotationDegrees16 = 0.0f;
	float ZrotationDegrees16 = 0.0f;
	glm::vec3 positionXYZ16;

	// set the XYZ scale for the mesh
	scaleXYZ16 = glm::vec3(9.0f, 2.50f, 0.01f);

	// set the XYZ rotation for the mesh
	XrotationDegrees16 = 0.0f;
	YrotationDegrees16 = 0.0f;
	ZrotationDegrees16 = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ16 = glm::vec3(13.0f, 1.5f, -2.49f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ16,
		XrotationDegrees16,
		YrotationDegrees16,
		ZrotationDegrees16,
		positionXYZ16);

	SetShaderColor(1, 1, 1, 1);

	// set the texture for the next draw command
	SetShaderTexture("magfront");

	// set the active material for the next draw command
	SetShaderMaterial("paper");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	/***Draw Box Mesh for Magazines   (top)                         */
	/****************************************************************/

	// declare the variables for the transformations
	glm::vec3 scaleXYZ17;
	float XrotationDegrees17 = 0.0f;
	float YrotationDegrees17 = 0.0f;
	float ZrotationDegrees17 = 0.0f;
	glm::vec3 positionXYZ17;

	// set the XYZ scale for the mesh
	scaleXYZ17 = glm::vec3(9.0f, 0.01f, 6.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees17 = 0.0f;
	YrotationDegrees17 = 0.0f;
	ZrotationDegrees17 = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ17 = glm::vec3(13.0f, 2.76f, -5.5f);


	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ17,
		XrotationDegrees17,
		YrotationDegrees17,
		ZrotationDegrees17,
		positionXYZ17);

	SetShaderColor(0.5f, 0.5f, 0.5f, 1.0f);

	// set the texture for the next draw command
	SetShaderTexture("magcover");

	// set the active material for the next draw command
	SetShaderMaterial("paper");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	/***Draw Box Mesh for top of keyboard   (keyboardtop)           */
	/****************************************************************/

	// declare the variables for the transformations
	glm::vec3 scaleXYZ18;
	float XrotationDegrees18 = 0.0f;
	float YrotationDegrees18 = 0.0f;
	float ZrotationDegrees18 = 0.0f;
	glm::vec3 positionXYZ18;

	// set the XYZ scale for the mesh
	scaleXYZ18 = glm::vec3(8.0f, 0.01f, 4.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees18 = 0.0f;
	YrotationDegrees18 = 0.0f;
	ZrotationDegrees18 = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ18 = glm::vec3(0.0f, 0.51f, 6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ18,
		XrotationDegrees18,
		YrotationDegrees18,
		ZrotationDegrees18,
		positionXYZ18);

	SetShaderColor(0.5f, 0.5f, 0.5f, 1.0f);

	// set the texture for the next draw command
	SetShaderTexture("keyboard");

	// set the active material for the next draw command
	SetShaderMaterial("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	/***Draw Cylinder Mesh for top of mouse   (mouse)               */
	/****************************************************************/

	// declare the variables for the transformations
	glm::vec3 scaleXYZ19;
	float XrotationDegrees19 = 0.0f;
	float YrotationDegrees19 = 0.0f;
	float ZrotationDegrees19 = 0.0f;
	glm::vec3 positionXYZ19;

	// set the XYZ scale for the mesh
	scaleXYZ19 = glm::vec3(1.0f, 0.15f, 1.25f);

	// set the XYZ rotation for the mesh
	XrotationDegrees19 = 0.0f;
	YrotationDegrees19 = 0.0f;
	ZrotationDegrees19 = 0.0f;
	
	// set the XYZ position for the mesh
	positionXYZ19 = glm::vec3(10.0f, 0.25f, 6.75f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ19,
		XrotationDegrees19,
		YrotationDegrees19,
		ZrotationDegrees19,
		positionXYZ19);

	SetShaderColor(0.8f, 0.8f, 0.8f, 1.0f);

	// set the texture for the next draw command
	SetShaderTexture("mousetop");

	// set the active material for the next draw command
	SetShaderMaterial("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
}
