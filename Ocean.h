#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cstdlib>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "myShader.h"
#include "myCamera.h"

const int N = 512;
//const int L = 512;
const float spacing = 10.0;
const int BUTTERFLY_STEPS = std::log2(N);
const int LOCAL_WORK_GROUP_SIZE = 32;
const int GLOBAL_WORK_GROUP_SIZE = N / LOCAL_WORK_GROUP_SIZE;

const float G = 9.81;
const float A = 0.001;//phillips�ײ�����Ӱ�첨�˸߶�
const float PI = 3.141592653589;
const glm::vec2 WindDir = glm::vec2(2.0, 1.5);//����

class Ocean {
public:
	Ocean()
	{
		show = true;
		diffuse = glm::vec3(0.0627f, 0.145f, 0.3f);
		specular = 1.0f;
		transparency = 0.8f;
		heightShader = new myShader("shader/ocean/0_1_GenerateHeightSpectrum.comp");
		displacementShader = new myShader("shader/ocean/0_2_GenerateXZDisplacement.comp");
		normalShader = new myShader("shader/ocean/0_3_GenerationNormalBubbles.comp");
		fftHorShader = new myShader("shader/ocean/0_4_FFTHorizontal.comp");
		fftVerShader = new myShader("shader/ocean/0_5_FFTVertical.comp");

		float *vertices = new float[N * N * 5];
		float sX = -(N - 1) * spacing / 2.0;
		float sZ = (N - 1) * spacing / 2.0;
		float vX = 0.0;
		float vY = 0.0;
		for (int i = 0; i < N; i++)
		{
			for (int j = 0; j < N; j++)
			{
				vertices[i * N * 5 + j * 5] = sX;
				vertices[i * N * 5 + j * 5 + 1] = 0.0;
				vertices[i * N * 5 + j * 5 + 2] = sZ;

				vertices[i * N * 5 + j * 5 + 3] = vX;
				vertices[i * N * 5 + j * 5 + 4] = vY;

				sX += spacing;

				vX += 1.0 / (N - 1);
			}
			sX = -(N - 1) * spacing / 2.0;
			sZ -= spacing;

			vX = 0.0;
			vY += 1.0 / (N - 1);
		}

		unsigned int* indices = new unsigned int[(N - 1) * (N - 1) * 6];
		for (int i = 0; i < N - 1; i++)
		{
			for (int j = 0; j < N - 1; j++)
			{
				indices[i * (N - 1) * 6 + j * 6] = i * N + j;
				indices[i * (N - 1) * 6 + j * 6 + 1] = i * N + j + 1;
				indices[i * (N - 1) * 6 + j * 6 + 2] = (i + 1) * N + j;
				indices[i * (N - 1) * 6 + j * 6 + 3] = i * N + j + 1;
				indices[i * (N - 1) * 6 + j * 6 + 4] = (i + 1) * N + j + 1;
				indices[i * (N - 1) * 6 + j * 6 + 5] = (i + 1) * N + j;
			}
		}

		glGenVertexArrays(1, &VAO);
		glBindVertexArray(VAO);
		unsigned int VBO;
		glGenBuffers(1, &VBO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, N * N * 5 * 4, vertices, GL_STATIC_DRAW);

		unsigned int EBO;
		glGenBuffers(1, &EBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, (N - 1) * (N - 1) * 6 * 4, indices, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);



		///
		float* h0Data = new float[N * N * 2];
		float* h0ConjData = new float[N * N * 2];
		for (int i = 0; i < N; i++)
		{
			for (int j = 0; j < N; j++)
			{
				//glm::vec2 k(2.0f * PI * (i - N / 2) / L, 2.0f * PI * (j - N / 2) / L);
				glm::vec2 k(2.0f * PI * i / N - PI, 2.0f * PI * j / N - PI);

				glm::vec2 gaussian = ComputeGaussianRandom(i, j);
				glm::vec2 hTilde0 = gaussian * std::sqrt(std::abs(GeneratePhillipsSpectrum(k) * DonelanBannerDirectionalSpreading(k)) / 2.0f);
				//gaussian = GenerateGaussianRandom();//�п�����Ҫ��ͬ�������
				glm::vec2 hTilde0Conj = gaussian * std::sqrt(std::abs(GeneratePhillipsSpectrum(-k) * DonelanBannerDirectionalSpreading(-k)) / 2.0f);
				hTilde0Conj.y *= -1.0f;//��������yΪ��

				h0Data[i * 2 * N + j * 2] = hTilde0.x;
				h0Data[i * 2 * N + j * 2 + 1] = hTilde0.y;

				h0ConjData[i * 2 * N + j * 2] = hTilde0Conj.x;
				h0ConjData[i * 2 * N + j * 2 + 1] = hTilde0Conj.y;
			}
		}

		//�洢H0
		glGenTextures(1, &g_textureH0);
		//glActiveTexture(GL_TEXTURE0); 
		glBindTexture(GL_TEXTURE_2D, g_textureH0);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, N, N, 0, GL_RG, GL_FLOAT, h0Data);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		//�洢H0Conj
		glGenTextures(1, &g_textureH0Conj);
		glBindTexture(GL_TEXTURE_2D, g_textureH0Conj);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, N, N, 0, GL_RG, GL_FLOAT, h0ConjData);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		//�洢Ht
		glGenTextures(1, &g_textureHt);
		glBindTexture(GL_TEXTURE_2D, g_textureHt);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, N, N, 0, GL_RG, GL_FLOAT, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		//�洢xz����ͼ��
		glGenTextures(1, &g_textureDisplacement[0]);//x
		glBindTexture(GL_TEXTURE_2D, g_textureDisplacement[0]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, N, N, 0, GL_RG, GL_FLOAT, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glGenTextures(1, &g_textureDisplacement[1]);//z
		glBindTexture(GL_TEXTURE_2D, g_textureDisplacement[1]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, N, N, 0, GL_RG, GL_FLOAT, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		//�洢IFFT���xyzƫ�ƽ��
		glGenTextures(1, &g_textureResult[0]);//x
		glBindTexture(GL_TEXTURE_2D, g_textureResult[0]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, N, N, 0, GL_RG, GL_FLOAT, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glGenTextures(1, &g_textureResult[1]);//y
		glBindTexture(GL_TEXTURE_2D, g_textureResult[1]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, N, N, 0, GL_RG, GL_FLOAT, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glGenTextures(1, &g_textureResult[2]);//z
		glBindTexture(GL_TEXTURE_2D, g_textureResult[2]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, N, N, 0, GL_RG, GL_FLOAT, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		//��ʱ�洢
		glGenTextures(1, &g_textureTemp);
		glBindTexture(GL_TEXTURE_2D, g_textureTemp);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, N, N, 0, GL_RG, GL_FLOAT, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		//�洢��������ֵ
		glGenTextures(1, &g_textureNormal);
		glBindTexture(GL_TEXTURE_2D, g_textureNormal);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, N, N, 0, GL_RGBA, GL_FLOAT, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		//ʹ��GPU����FFT��Ҫ�Ȱѵ�һ�ε������������������N=8ʱ ����Ϊ(0,4) (2,6) (1,5) (3,7)
		float* butterflyIndices = new float[N];
		for (int i = 0; i < N / 4; i++)
		{
			butterflyIndices[i * 2] = i * 2;
			butterflyIndices[i * 2 + 1] = i * 2 + N / 2;

			butterflyIndices[i * 2 + N / 2] = i * 2 + 1;
			butterflyIndices[i * 2 + N / 2 + 1] = i * 2 + N / 2 + 1;
		}

		//������õ�����ֵ�洢����ͼ����
		glGenTextures(1, &g_textureIndices);
		glBindTexture(GL_TEXTURE_1D, g_textureIndices);
		glTexImage1D(GL_TEXTURE_1D, 0, GL_R32F, N, 0, GL_RED, GL_FLOAT, butterflyIndices);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		glEnable(GL_DEPTH_TEST);
	}

	void calculate(float timeValue, myCamera *cam) {
		timeValue /= 150;
		//ʹ��HeightSpectrum������ɫ������߶�
		heightShader->use();
		glBindImageTexture(0, g_textureH0, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RG32F);
		glBindImageTexture(1, g_textureH0Conj, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RG32F);
		glBindImageTexture(2, g_textureHt, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG32F);
		//��������ʱ��
		heightShader->setFloat("time", timeValue);
		//����һ��N*N��С�Ĺ����飬��ͬʱ�������еĶ���߶�ֵ
		glDispatchCompute(GLOBAL_WORK_GROUP_SIZE, GLOBAL_WORK_GROUP_SIZE, 1);
		//ȷ�����е����ݶ�д�뵽��ͼ����
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		//ʹ��XZDisplacement������ɫ������xz����ƫ��
		displacementShader->use();
		glBindImageTexture(0, g_textureHt, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RG32F);
		glBindImageTexture(1, g_textureDisplacement[0], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG32F);
		glBindImageTexture(2, g_textureDisplacement[1], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG32F);
		//����һ��N*N��С�Ĺ����飬��ͬʱ�������еĶ���߶�ֵ
		glDispatchCompute(GLOBAL_WORK_GROUP_SIZE, GLOBAL_WORK_GROUP_SIZE, 1);
		//ȷ�����е����ݶ�д�뵽��ͼ����
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		//ʹ��FFT������ɫ���Խ��������任
		//y
		//Horizontal
		fftHorShader->use();
		fftHorShader->setInt("u_steps", BUTTERFLY_STEPS);
		//��������ͼ���ϸ�������ɫ�����ý���Ĳ��˺�����ͼ����Ҫ�洢ƫ��ֵ����ͼ
		glBindImageTexture(0, g_textureHt, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RG32F);
		glBindImageTexture(1, g_textureTemp, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG32F);
		glBindImageTexture(2, g_textureIndices, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
		// �ȶ�ÿһ�н�����任
		glDispatchCompute(1, N, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		//Vertical
		fftVerShader->use();
		fftVerShader->setInt("u_steps", BUTTERFLY_STEPS);
		//��������ͼ���ϸ�������ɫ�����ý���Ĳ��˺�����ͼ����Ҫ�洢ƫ��ֵ����ͼ
		glBindImageTexture(0, g_textureTemp, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RG32F);
		glBindImageTexture(1, g_textureResult[1], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG32F);
		glBindImageTexture(2, g_textureIndices, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
		// �ٶ�ÿһ�н�����任
		glDispatchCompute(N, 1, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		//ʹ��FFT������ɫ���Խ��������任
		//x
		//Horizontal
		fftHorShader->use();
		fftHorShader->setInt("u_steps", BUTTERFLY_STEPS);
		//��������ͼ���ϸ�������ɫ�����ý���Ĳ��˺�����ͼ����Ҫ�洢ƫ��ֵ����ͼ
		glBindImageTexture(0, g_textureDisplacement[0], 0, GL_FALSE, 0, GL_READ_ONLY, GL_RG32F);
		glBindImageTexture(1, g_textureTemp, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG32F);
		glBindImageTexture(2, g_textureIndices, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
		// �ȶ�ÿһ�н�����任
		glDispatchCompute(1, N, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		//Vertical
		fftVerShader->use();
		fftVerShader->setInt("u_steps", BUTTERFLY_STEPS);
		//��������ͼ���ϸ�������ɫ�����ý���Ĳ��˺�����ͼ����Ҫ�洢ƫ��ֵ����ͼ
		glBindImageTexture(0, g_textureTemp, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RG32F);
		glBindImageTexture(1, g_textureResult[0], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG32F);
		glBindImageTexture(2, g_textureIndices, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
		// �ٶ�ÿһ�н�����任
		glDispatchCompute(N, 1, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		//ʹ��FFT������ɫ���Խ��������任
		//z
		//Horizontal
		fftHorShader->use();
		fftHorShader->setInt("u_steps", BUTTERFLY_STEPS);
		//��������ͼ���ϸ�������ɫ�����ý���Ĳ��˺�����ͼ����Ҫ�洢ƫ��ֵ����ͼ
		glBindImageTexture(0, g_textureDisplacement[1], 0, GL_FALSE, 0, GL_READ_ONLY, GL_RG32F);
		glBindImageTexture(1, g_textureTemp, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG32F);
		glBindImageTexture(2, g_textureIndices, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
		// �ȶ�ÿһ�н�����任
		glDispatchCompute(1, N, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		//Vertical
		fftVerShader->use();
		fftVerShader->setInt("u_steps", BUTTERFLY_STEPS);
		//��������ͼ���ϸ�������ɫ�����ý���Ĳ��˺�����ͼ����Ҫ�洢ƫ��ֵ����ͼ
		glBindImageTexture(0, g_textureTemp, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RG32F);
		glBindImageTexture(1, g_textureResult[2], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG32F);
		glBindImageTexture(2, g_textureIndices, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
		// �ٶ�ÿһ�н�����任
		glDispatchCompute(N, 1, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		//���·�������
		normalShader->use();
		glBindImageTexture(0, g_textureResult[0], 0, GL_FALSE, 0, GL_READ_ONLY, GL_RG32F);
		glBindImageTexture(1, g_textureResult[1], 0, GL_FALSE, 0, GL_READ_ONLY, GL_RG32F);
		glBindImageTexture(2, g_textureResult[2], 0, GL_FALSE, 0, GL_READ_ONLY, GL_RG32F);
		glBindImageTexture(3, g_textureNormal, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
		glDispatchCompute(GLOBAL_WORK_GROUP_SIZE, GLOBAL_WORK_GROUP_SIZE, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}

	void draw(unsigned int ReflectText) {
		//�����˸߶���ͼ����GL_TEXTURE0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, g_textureResult[0]);//x
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, g_textureResult[1]);//y
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, g_textureResult[2]);//z
		//��������ͼ����GL_TEXTURE3
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, g_textureNormal);

		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_CUBE_MAP, ReflectText);

		glBindVertexArray(VAO);
		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDrawElements(GL_TRIANGLES, (N - 1) * (N - 1) * 6, GL_UNSIGNED_INT, (void*)0);
	}

	float GeneratePhillipsSpectrum(glm::vec2 k)
	{
		float kLength = glm::length(k);
		kLength = std::max(0.001f, kLength);
		//kLength = 1;
		float kLength2 = kLength * kLength;
		float kLength4 = kLength2 * kLength2;

		//
		float k_dot_w = glm::dot(glm::normalize(k), glm::normalize(WindDir));
		float k_dot_w2 = k_dot_w * k_dot_w;
		//

		float windLength = glm::length(WindDir);
		float l = windLength * windLength / G;
		float l2 = l * l;

		//�������ӣ�Phillips��|k|�ϴ�ʱ�����Խϲʹ������exp�������������ƽ�С���ˣ�
		float damping = 0.001;
		float L2 = l2 * damping * damping;

		//Phillips��
		return  A * exp(-1.0f / (kLength2 * l2)) / kLength4 /** k_dot_w2*/ * exp(-kLength2 * L2);
		//*k_dot_w2Ϊ������չ�������滻ΪDonelan-Banner���򴫲�*
	}

	float dispersion(glm::vec2 k)
	{
		return std::sqrt(G * glm::length(k));
	}

	glm::vec2 ComputeGaussianRandom(int x, int y)
	{
		glm::vec2 g = gaussian(x, y);

		return g;
	}

	glm::vec2 gaussian(int x, int y)
	{
		//���ȷֲ������
		unsigned int rngState = wangHash(y * N + x);
		float x1 = rand(rngState);
		float x2 = rand(rngState);

		x1 = std::max(1e-6f, x1);
		x2 = std::max(1e-6f, x2);
		//���������໥�����ĸ�˹�����
		float g1 = sqrt(-2.0f * log(x1)) * cos(2.0f * PI * x2);
		float g2 = sqrt(-2.0f * log(x1)) * sin(2.0f * PI * x2);

		return glm::vec2(g1, g2);
	}

	unsigned int wangHash(unsigned int seed)
	{
		auto s = (seed ^ 61) ^ (seed >> 16);
		s *= 9;
		s = s ^ (s >> 4);
		s *= 0x27d4eb2d;
		s = s ^ (s >> 15);
		return s;
	}

	float rand(unsigned int& rngState)
	{
		// Xorshift�㷨
		rngState ^= (rngState << 13);
		rngState ^= (rngState >> 17);
		rngState ^= (rngState << 5);
		return rngState / 4294967296.0f;;
	}

	//Donelan-Banner������չ
	float DonelanBannerDirectionalSpreading(glm::vec2 k)
	{
		float betaS;
		float omegap = 0.855f * G / glm::length(WindDir);
		float ratio = dispersion(k) / omegap;

		if (ratio < 0.95f)
		{
			betaS = 2.61f * std::pow(ratio, 1.3f);
		}
		if (ratio >= 0.95f && ratio < 1.6f)
		{
			betaS = 2.28f * std::pow(ratio, -1.3f);
		}
		if (ratio > 1.6f)
		{
			float epsilon = -0.4f + 0.8393f * std::exp(-0.567f * std::log(ratio * ratio));
			betaS = std::pow(10, epsilon);
		}
		float theta = std::atan2(k.y, k.x) - std::atan2(WindDir.y, WindDir.x);

		return betaS / std::max(1e-7, 2.0f * std::tanh(betaS * PI) * std::pow(std::cosh(betaS * theta), 2));
	}

public:
		bool show;
		glm::vec3 diffuse;
		float specular;
		float transparency;

		float speed;
private:
	myShader* heightShader;
	myShader* displacementShader;
	myShader* normalShader;
	myShader* fftHorShader;
	myShader* fftVerShader;

	unsigned int g_textureH0;
	unsigned int g_textureH0Conj;
	unsigned int g_textureHt;
	unsigned int g_textureDisplacement[2];
	unsigned int g_textureResult[3];
	unsigned int g_textureTemp;
	unsigned int g_textureNormal;
	unsigned int g_textureIndices;

	unsigned int VAO;

};