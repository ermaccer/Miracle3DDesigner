#pragma once
#include "Miracle3D.h"
#include <iostream>
#include <string>
#include <vector>
#include <Windows.h>
#include <fstream>
#include <time.h>



#define M3D_DESIGNER_VERSION L"0.8.0b"


struct Miracle3DModel {

	int type;
	std::vector<v_data> Verts;
	std::vector<uv> Maps;
	std::vector<vn> Normals;
	std::vector<face> Faces;


	std::string Name;
};


struct Miracle3DDecompiledModel {
	int type;
	int animationNumber;
	std::wstring TextureName;
	std::wstring AnimatedData;
	std::wstring Model;

};


struct VecDuplicate {
	int   prim;
	int   copy;
};

class M3DManager {
private:
	HWND* hTextureNameBox;
	HWND* hLogBox;
	HWND* hFileNameBox;
	HWND* hGroupBox;
	HWND* hCompilePath;
public:
	std::ifstream pFile;
	std::wstring InputPath;
	std::wstring OutputPath;
	model_info	 ModelInfo;
	static_model_header sModelInfo;
	Miracle3DDecompiledModel ModelConfig;
	int ModelType;
	int WorkType;
	std::vector<m3d_section> Sections;
	std::vector<bone_info>   Skeleton;
	std::vector<Miracle3DModel> Models;


	void         Init(HWND* list, HWND* box, HWND* key, HWND* table);

	void OpenFile(std::wstring input);
	void OpenINI(std::wstring input);
	void ReadFile();
	void ReadINI();
	void Close();
	void Log(std::wstring msg);
	void ExportToSMD(std::wstring folder, bool combine, bool dontFlipUV = true);
	void ExportToOBJ(std::wstring folder, bool combine, bool dontFlipUV = true);
	void OpenSMD(std::wstring file);
	void Compile(std::wstring file, bool dontFlipUV = true);
	void OptimizeForCompilation();
	void Decompile(std::wstring folder);

	// gui
	void SetTextureText(std::wstring text);
	
	// models
	int GetFirstAnimatedModel();
	int GetBoneID(std::string name);
	
	int Get3DDataSize(Miracle3DModel model);
	int GetVertDataSize(Miracle3DModel model);
	int GetFaceDataSize(Miracle3DModel model);
};

bool IsValidM3DFile(int id);
char* GetModelInfo(int id);
wchar_t* GetModelType(int id);

std::wstring   SetPathFromButton(wchar_t* filter, wchar_t* ext, HWND hWnd);
std::wstring   SetSavePathFromButton(wchar_t* filter, wchar_t* ext, HWND hWnd);
std::wstring   SetFolderFromButton(HWND hWnd);
void		   PushLogMessage(HWND hWnd, std::wstring msg);

v_data	       FindFirstVertex(std::vector<v_data> data, v_data v);
int 	       FindFirstVertexNumber(std::vector<v_data> data, v_data v);