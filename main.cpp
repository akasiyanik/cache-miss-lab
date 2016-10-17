#include <iostream>
#include <random>
#include <chrono>

using namespace std;

//__declspec(align(16))
// __restrict
namespace {
    class Cache {
    public:
        long long hitCounter = 0;
        long long missCounter = 0;

        Cache(int cacheLineSize, int channels, int size) : cacheLineSize(cacheLineSize), channels(channels),
                                                           size(size) {
            totalLinesInCache = size / cacheLineSize;
            linesInChannel = totalLinesInCache / channels;

            cout << "cache size " << size << " B" << endl;
            cout << "channels " << channels << endl;
            cout << "cacheLineSize " << cacheLineSize << " B" << endl;
            cout << "totalLinesInCache " << totalLinesInCache << endl;
            cout << "linesInChannel " << linesInChannel << endl;

            cache = new long long[totalLinesInCache];
            lastAccessTime = new long long[totalLinesInCache];
            for (int i = 0; i < totalLinesInCache; i++) {
                cache[i] = -1;
                lastAccessTime[i] = -1;
            }
        }

        virtual ~Cache() {
            delete[] cache;
            delete[] lastAccessTime;

        }

        void access(long long addrInMem) {
            long long lineNumberInMem = getCacheLineNumber(addrInMem);
            int lineTag = (int) (lineNumberInMem % linesInChannel);

            if (isCacheContain(lineTag, lineNumberInMem)) {
                hitCounter++;
            } else {
                missCounter++;
                addLineToCache(lineTag, lineNumberInMem);
            }
            return;
        }

    private:
        int cacheLineSize = 64;
        int channels = 6;
        int size = 32 * 1024;

        int linesInChannel;
        int totalLinesInCache;

        long long *cache;
        long long *lastAccessTime;

        long long getCurrentTimestamp() {
            return chrono::system_clock::now().time_since_epoch().count();
        }

        bool isCacheContain(int cacheLineTag, long long cacheLineNumberInMem) {
            for (int i = 0; i < channels; i++) {
                long n = cacheLineTag * channels + i;
                if (cache[n] == cacheLineNumberInMem) {
                    lastAccessTime[n] = getCurrentTimestamp();
                    return true;
                }
            }
            return false;
        }

        void addLineToCache(int cacheLineTag, long long cacheLineNumberInMem) {
            long lruLineNum = cacheLineTag * channels;
            long minAccessTime = lastAccessTime[lruLineNum];
            for (int i = 1; i < channels; i++) {
                long n = cacheLineTag * channels + i;
                if (lastAccessTime[n] < minAccessTime) {
                    minAccessTime = lastAccessTime[n];
                    lruLineNum = n;
                }
            }

            cache[lruLineNum] = cacheLineNumberInMem;
            lastAccessTime[lruLineNum] = getCurrentTimestamp();
        }

        long long getCacheLineNumber(long long addrInMem) {
            return addrInMem / cacheLineSize;
        }

    };

    void MultSimple(const float *__restrict a, const float *__restrict b, float *__restrict c, int n, Cache &cache) {

        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                c[i * n + j] = 0.f;
                for (int k = 0; k < n; ++k) {
                    cache.access((long long) &a[i * n + k]);
                    cache.access((long long) &b[k * n + j]);
                    cache.access((long long) &c[i * n + j]);
                    c[i * n + j] += a[i * n + k] * b[k * n + j];
                }
            }
        }
    }

    void FillRandom(float *a, int n) {
        std::default_random_engine eng;
        std::uniform_real_distribution<float> dist;

        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                a[i * n + j] = dist(eng);
            }
        }
    }


}

int main(int argc, char *argv[]) {
    const int n = atoi(argv[1]);

    const int cacheLineSize = atoi(argv[2]);
    const int channels = atoi(argv[3]);
    const int size = atoi(argv[4]);

    cerr << "n = " << n << std::endl;

    float *a = new float[n * n];
    float *b = new float[n * n];
    float *c = new float[n * n];

    FillRandom(a, n);
    FillRandom(b, n);

    {
        Cache cache(cacheLineSize, channels, size);
        const auto startTime = std::clock();
        MultSimple(a, b, c, n, cache);
        const auto endTime = std::clock();

        cerr << "timeSimple: " << double(endTime - startTime) / CLOCKS_PER_SEC << '\n';

        cerr << "TOTAL " << cache.hitCounter + cache.missCounter << endl;
        cerr << "HIT " << cache.hitCounter << endl;
        cerr << "MISS " << cache.missCounter << endl;
        cerr << "MISS % " << cache.missCounter / (float) (cache.hitCounter + cache.missCounter) << endl;
    }

    delete[] a;
    delete[] b;
    delete[] c;
}

