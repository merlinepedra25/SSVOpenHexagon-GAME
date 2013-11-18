// Copyright (c) 2013 Vittorio Romeo
// License: Academic Free License ("AFL") v. 3.0
// AFL License page: http://opensource.org/licenses/AFL-3.0

#include <dirent.h>
#include <sys/stat.h>
#include <fstream>
#include "SSVOpenHexagon/Utils/Utils.hpp"
#include "SSVOpenHexagon/Global/Config.hpp"

using namespace std;
using namespace sf;
using namespace ssvs;
using namespace hg::Utils;
using namespace ssvuj;
using namespace ssvu::FileSystem;
using namespace ssvu;

namespace hg
{
	namespace Utils
	{
		Color getColorFromHue(double mHue)
		{
			double s{1}, v{1}, r{0}, g{0}, b{0};
			int i(floor(mHue * 6));
			double f{mHue * 6 - i}, p{v * (1 - s)}, q{v * (1 - f * s)}, t{v * (1 - (1 - f) * s)};

			switch(i % 6)
			{
				case 0: r = v, g = t, b = p; break;
				case 1: r = q, g = v, b = p; break;
				case 2: r = p, g = v, b = t; break;
				case 3: r = p, g = q, b = v; break;
				case 4: r = t, g = p, b = v; break;
				case 5: r = v, g = p, b = q; break;
			}

			return Color(r * 255, g * 255, b * 255, 255);
		}
		Color getColorDarkened(Color mColor, float mMultiplier) { mColor.r /= mMultiplier; mColor.b /= mMultiplier; mColor.g /= mMultiplier; return mColor; }

		MusicData loadMusicFromJson(const ssvuj::Obj& mRoot)
		{
			MusicData result{as<string>(mRoot, "id"), as<string>(mRoot, "file_name"), as<string>(mRoot, "name"), as<string>(mRoot, "album"), as<string>(mRoot, "author")};
			for(const auto& segment : mRoot["segments"]) result.addSegment(as<int>(segment, "time"));
			return result;
		}
		ProfileData loadProfileFromJson(const ssvuj::Obj& mRoot) { return {as<float>(mRoot, "version"), as<string>(mRoot, "name"), mRoot["scores"], as<vector<string>>(mRoot, "trackedNames", {})}; }

		string getLocalValidator(const string& mId, float mDifficultyMult) { return mId + "_m_" + toStr(mDifficultyMult); }

		void shakeCamera(TimelineManager& mTimelineManager, Camera& mCamera)
		{
			int s{7};
			Vec2f oldCenter{mCamera.getCenter()};
			Timeline& timeline(mTimelineManager.create());

			for(int i{s}; i > 0; --i)
			{
				timeline.append<Do>([&mCamera, oldCenter, i]{ mCamera.setCenter(oldCenter + Vec2f(getRnd(-i, i), getRnd(-i, i))); });
				timeline.append<Wait>(1); timeline.append<Go>(0, 3);
			}

			timeline.append<Do>([&mCamera, oldCenter]{ mCamera.setCenter(oldCenter); });
		}

		std::set<string> getIncludedLuaFileNames(const string& mLuaScript)
		{
			string script{mLuaScript}, toFind{"execScript("};
			std::set<string> result;

			size_t pos{script.find(toFind, 0)};
			while(pos != string::npos)
			{
				size_t startPos{pos};
				string untilEnd{script.substr(startPos + toFind.size() + 1, script.size() - startPos)};
				size_t lastPos{untilEnd.find("\"", 0)};
				string luaFileName{untilEnd.substr(0, lastPos)};

				result.insert(luaFileName);
				pos = script.find(toFind, pos + 1);
			}

			return result;
		}
		void recursiveFillIncludedLuaFileNames(std::set<string>& mLuaScriptNames, const Path& mPackPath, const string& mLuaScript)
		{
			for(const auto& name : getIncludedLuaFileNames(mLuaScript))
			{
				mLuaScriptNames.insert(name);
				recursiveFillIncludedLuaFileNames(mLuaScriptNames, mPackPath, getFileContents(mPackPath + "/Scripts/" + name));
			}
		}

		Color transformHue(const Color& in, float H)
		{
			float u{cos(H * 3.14f / 180.f)};
			float w{sin(H * 3.14f / 180.f)};

			Color ret;
			ret.r = (.701*u+.168*w)*in.r + (-.587*u+.330*w)*in.g + (-.114*u-.497*w)*in.b;
			ret.g = (-.299*u-.328*w)*in.r + (.413*u+.035*w)*in.g + (-.114*u+.292*w)*in.b;
			ret.b = (-.3*u+1.25*w)*in.r + (-.588*u-1.05*w)*in.g + (.886*u-.203*w)*in.b;
			return ret;
		}
	}
}
