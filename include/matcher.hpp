#ifndef BOW_HPP
#define BOW_HPP

#include "instrumentation.hpp"
#include "database_iface.hpp"
#include <optional>

template <typename It>
struct ItAlignment
{
    It startKnown, startUnknown, endKnown, endUnknown;
    unsigned int score;

    template <typename = std::void_t<std::is_integral<It>>>
    operator std::string()
    {
        return std::to_string(startKnown) + " " + std::to_string(endKnown) +
               " " + std::to_string(startUnknown) + " " + std::to_string(endUnknown) + " " +
               std::to_string(score);
    }
};

typedef ItAlignment<unsigned int> Alignment;

struct MatchInfo
{
    double matchConfidence;
    IVideo::size_type startFrame, endFrame;
    std::string video;
    std::vector<Alignment> alignments;
};

template <typename Matrix>
double cosineSimilarity(Matrix &&b1, Matrix &&b2)
{
    if (b1.empty() || b1.size() != b2.size())
        return -1;

    auto b1n = b1.dot(b1);
    auto b2n = b2.dot(b2);

    return b1.dot(b2) / (sqrt(b1n * b2n) + 1e-10);
}

template <typename Extractor>
double frameSimilarity(Frame &f1, Frame &f2, Extractor &&extractor)
{
    auto b1 = extractor(f1), b2 = extractor(f2);

    return cosineSimilarity(b1, b2);
}

namespace cv
{
class Mat;
}

class ColorComparator
{
public:
    double operator()(const Frame &f1, const Frame &f2) const;
    double operator()(const cv::Mat &, const cv::Mat &) const;
};

class QueryVideo : public IVideo
{
    std::unique_ptr<ICursor<SerializableScene>> scenes;

public:
    QueryVideo(const IVideo &source, std::unique_ptr<ICursor<SerializableScene>> scenes)
        : IVideo(source), scenes(std::move(scenes)){};

    std::unique_ptr<ICursor<SerializableScene>> getScenes() { return std::move(scenes); };
};

class FileDatabase;
class DatabaseVideo;
class SIFTVideo;

QueryVideo make_query_adapter(const SIFTVideo&);
QueryVideo make_query_adapter(const DatabaseVideo&);

std::optional<MatchInfo> findMatch(QueryVideo &target, FileDatabase &db);

#endif
