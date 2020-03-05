#ifndef DATABASE_H
#define DATABASE_H
#include "frame.hpp"
#include <vector>
#include <memory>
#include <string>
#include <opencv2/opencv.hpp>
#include <functional>
#include <experimental/filesystem>
#include <type_traits>
#include <optional>
#include "database_iface.hpp"
#include <fstream>
#include <variant>

#define CONFIG_FILE

namespace fs = std::experimental::filesystem;

std::string getAlphas(const std::string& input);
void createFolder(const std::string& folder_name);
cv::Mat scaleToTarget(cv::Mat image, int targetWidth, int targetHeight);

struct RuntimeArguments {
    int KScenes;
    int KFrame;
    double threshold;
};

struct Configuration {
    int KScenes;
    int KFrames;
    double threshold;
    StrategyType strategy;

    Configuration() : KScenes(-1), KFrames(-1), threshold(-1) {};
    Configuration(const RuntimeArguments& args, StrategyType type)
        : KScenes(args.KScenes), KFrames(args.KFrame), threshold(args.threshold), strategy(type) {};
};

template<typename Base>
class InputVideoAdapter : public IVideo {
private:
    Base base;
    std::vector<SerializableScene> emptyScenes;
public:
    using size_type = typename Base::size_type;

    explicit InputVideoAdapter(Base&& b, const std::string& name) : IVideo(name), base(std::forward<Base>(b)) {};
    explicit InputVideoAdapter(const Base& b, const std::string& name) : IVideo(name), base(std::forward<Base>(b)) {};
    InputVideoAdapter(IVideo&& vid) : IVideo(vid), base(vid.frames()) {};
    InputVideoAdapter(IVideo& vid) : IVideo(vid), base(vid.frames()) {};
    size_type frameCount() override { return base.frameCount(); };
    std::vector<Frame>& frames() & override { return base.frames(); };
    std::vector<SerializableScene>& getScenes() & override { return emptyScenes; }
};

class FileLoader {
private:
    fs::path rootDir;
public:
    explicit FileLoader(fs::path dir) : rootDir(dir) {};

    std::optional<Frame> readFrame(const std::string& videoName, SIFTVideo::size_type index) const;
    std::optional<SerializableScene> readScene(const std::string& videoName, SIFTVideo::size_type index) const;
};

template<typename Base>
InputVideoAdapter<Base> make_video_adapter(Base&& b, const std::string& name) {
    return InputVideoAdapter<Base>(std::forward<Base>(b), name);
}


// TODO think of how to use these, templated
/* Example strategies
class IVideoLoadStrategy {
public:
    virtual std::unique_ptr<IVideo> operator()(const std::string& findKey) const = 0;
    virtual ~IVideoLoadStrategy() = default;
};

 */

class AggressiveStorageStrategy : public IVideoStorageStrategy {
public:
    inline StrategyType getType() const { return Eager; };
    inline bool shouldBaggifyFrames(IVideo& video) override { return true; };
    inline bool shouldComputeScenes(IVideo& video) override { return true; };
    inline bool shouldBaggifyScenes(IVideo& video) override { return true; };
};

class LazyStorageStrategy : public IVideoStorageStrategy {
public:
    inline StrategyType getType() const { return Lazy; };
    inline bool shouldBaggifyFrames(IVideo& video) override { return false; };
    inline bool shouldComputeScenes(IVideo& video) override { return false; };
    inline bool shouldBaggifyScenes(IVideo& video) override { return false; };
};

class AggressiveLoadStrategy {};

class LazyLoadStrategy {};

class FileDatabase {
private:
    fs::path databaseRoot;
    std::vector<std::unique_ptr<IVideo>> loadVideos() const;
    std::unique_ptr<IVideoStorageStrategy> strategy;
    Configuration config;
    FileLoader loader;

    typedef std::variant<LazyLoadStrategy, AggressiveLoadStrategy> LoadStrategy;
    LoadStrategy loadStrategy;
public:
    explicit FileDatabase(std::unique_ptr<IVideoStorageStrategy>&& strat, LoadStrategy l, RuntimeArguments args) :
    FileDatabase(fs::current_path() / "database", std::move(strat), l, args) {};

    explicit FileDatabase(const std::string& databasePath,
        std::unique_ptr<IVideoStorageStrategy>&& strat, LoadStrategy l, RuntimeArguments args)
        : strategy(std::move(strat)), loadStrategy(l), config(args, strategy->getType()), databaseRoot(databasePath),
        loader(databasePath) {
            if(!fs::exists(databaseRoot)) {
                fs::create_directories(databaseRoot);
            }
            Configuration configFromFile;
            std::ifstream reader(databaseRoot / "config.bin", std::ifstream::binary);
            if(reader.is_open()) {
                reader.read((char*)&configFromFile, sizeof(configFromFile));
                if(config.threshold == -1) {
                    config.threshold = configFromFile.threshold;
                }
                if(config.KScenes == -1) {
                    config.KScenes = configFromFile.KScenes;
                }
                if(config.KFrames == -1) {
                    config.KFrames = configFromFile.KFrames;
                }
            }

            std::ofstream writer(databaseRoot / "config.bin", std::ofstream::binary);
            writer.write((char*)&config, sizeof(config));
        };

    std::unique_ptr<IVideo> saveVideo(IVideo& video);
    std::unique_ptr<IVideo> loadVideo(const std::string& key = "") const;
    std::vector<std::string> listVideos() const;

    const FileLoader& getFileLoader() const & { return loader; };
    const Configuration& getConfig() const & { return config; };

    bool saveVocab(const ContainerVocab& vocab, const std::string& key);
    std::optional<ContainerVocab> loadVocab(const std::string& key) const;
};

class DatabaseVideo : public IVideo {
    const FileDatabase& db;
    std::vector<SerializableScene> sceneCache;
    std::vector<Frame> frameCache;
public:
    DatabaseVideo() = delete;
    explicit DatabaseVideo(const FileDatabase& database, const std::string& key) : 
    DatabaseVideo(database, key, {}, {}) {};
    explicit DatabaseVideo(const FileDatabase& database, const std::string& key, const std::vector<Frame>& frames) :
    DatabaseVideo(database, key, frames, {}) {};
    explicit DatabaseVideo(const FileDatabase& database, const std::string& key, const std::vector<SerializableScene> scenes) :
    DatabaseVideo(database, key, {}, scenes) {};
    explicit DatabaseVideo(const FileDatabase& database, const std::string& key, const std::vector<Frame>& frames, const std::vector<SerializableScene>& scenes) : IVideo(key),
    db(database), frameCache(frames), sceneCache(scenes) {};


    inline size_type frameCount() override { return frameCache.size(); };
    std::vector<Frame>& frames() & override;

    std::vector<SerializableScene>& getScenes() & override;
};

inline std::unique_ptr<FileDatabase> database_factory(const std::string& dbPath, int KFrame, int KScene, double threshold) {
    return std::make_unique<FileDatabase>(dbPath,
        std::make_unique<AggressiveStorageStrategy>(),
        AggressiveLoadStrategy{},
        RuntimeArguments{KScene, KFrame, threshold});
}

inline std::unique_ptr<FileDatabase> query_database_factory(const std::string& dbPath, int KFrame, int KScene, double threshold) {
    return std::make_unique<FileDatabase>(dbPath,
        std::make_unique<AggressiveStorageStrategy>(),
        LazyLoadStrategy{},
        RuntimeArguments{KScene, KFrame, threshold});
}


DatabaseVideo make_scene_adapter(FileDatabase& db, IVideo& video, const std::string& key);

template<typename Video>
inline DatabaseVideo make_query_adapter(FileDatabase& db, Video&& video, const std::string& key) {
    auto frames = video.frames();
    return DatabaseVideo(db, key, frames);
}

#endif
