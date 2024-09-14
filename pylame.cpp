#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <iostream>
extern "C"
{
#include <lame/lame.h>
}

namespace py = pybind11;

static void silentOutput(const char *format, va_list ap)
{
    return;
}

class Encoder
{
public:
    Encoder()
    {
        lame = lame_init();
        if (lame == nullptr)
        {
            throw std::runtime_error("Failed to initialize LAME encoder");
        }
        lame_set_num_channels(lame, 1);
        lame_set_in_samplerate(lame, 22050);
        lame_set_brate(lame, 128);
        lame_set_quality(lame, 5);
        lame_set_bWriteVbrTag(lame, 0);
        silence();
        initialised = 0;
        lame_set_VBR(lame, vbr_default);  
    }

    ~Encoder()
    {
        lame_close(lame);
    }

    void set_channels(int channels)
    {
        if (lame_set_num_channels(lame, channels) < 0)
        {
            throw std::runtime_error("Unable to set the channels");
        }
    }

    void set_bitrate(int bitrate)
    {
        if (lame_set_brate(lame, bitrate) < 0)
        {
            throw std::runtime_error("Unable to set the bit rate");
        }
    }

    void set_in_sample_rate(int rate)
    {
        if (lame_set_in_samplerate(lame, rate) < 0)
        {
            throw std::runtime_error("Unable to set the input sample rate");
        }
    }

    void set_out_sample_rate(int rate)
    {
        if (lame_set_out_samplerate(lame, rate) < 0)
        {
            throw std::runtime_error("Unable to set the output sample rate");
        }
    }

    void set_quality(int quality)
    {
        if (lame_set_quality(lame, quality) < 0)
        {
            throw std::runtime_error("Unable to set the quality");
        }
    }

    void silence()
    {
        if (lame_set_errorf(lame, &silentOutput) < 0 ||
            lame_set_debugf(lame, &silentOutput) < 0 ||
            lame_set_msgf(lame, &silentOutput) < 0)
        {
            throw std::runtime_error("Unable to redirect output to silent function");
        }
    }

    void init_encoder(int channels)
    {
        int ret;
        {
            py::gil_scoped_release release;
            if (channels == 1)
            {
                lame_set_mode(lame, MONO);
            }
            else if (lame_get_brate(lame) > 128)
            {
                lame_set_mode(lame, STEREO);
            }
            ret = lame_init_params(lame);
        }

        if (ret >= 0)
        {
            initialised = 1;
        }
        else
        {
            throw std::runtime_error("Error initializing the encoder");
        }
    }

    py::bytes encode(py::array_t<short> pcm_data)
    {
        if (!pcm_data.ndim() == 1 || pcm_data.size() == 0)
        {
            throw std::runtime_error("Input data must be a 1D array of 16-bit PCM samples");
        }

        int channels = lame_get_num_channels(lame);

        if (initialised == 0)
        {
            init_encoder(channels);
        }

        if (initialised != 1)
        {
            throw std::runtime_error("Encoder not initialised");
        }

        short *pcm = pcm_data.mutable_data();

        size_t sampleCount = pcm_data.size() / channels;
        if (pcm_data.size() % channels != 0)
        {
            throw std::runtime_error("The input data must be interleaved 16-bit PCM");
        }

        size_t requiredOutputBytes = sampleCount + (sampleCount / 4) + 7200;
        std::vector<unsigned char> outputBuffer(requiredOutputBytes);

        int outputBytes = 0;
        {
            py::gil_scoped_release releaseL;
            if (channels > 1)
            {
                outputBytes = lame_encode_buffer_interleaved(
                    lame,
                    pcm, static_cast<int>(sampleCount),
                    outputBuffer.data(), static_cast<int>(requiredOutputBytes));
            }
            else
            {
                outputBytes = lame_encode_buffer(
                    lame,
                    pcm, pcm, static_cast<int>(sampleCount),
                    outputBuffer.data(), static_cast<int>(requiredOutputBytes));
            }
        }

        if (outputBytes < 0)
        {
            throw std::runtime_error("LAME encoding error");
        }

        return py::bytes(reinterpret_cast<const char *>(outputBuffer.data()), outputBytes);
    }

    py::bytes flush()
    {
        if (initialised != 1)
        {
            throw std::runtime_error("Not currently encoding");
        }
        static const int blockSize = 8 * 1024;
        std::vector<unsigned char> outputBuffer(blockSize);

        int bytes = 0;
        {
            py::gil_scoped_release release;
            bytes = lame_encode_flush(lame, outputBuffer.data(), blockSize);
        }

        if (bytes > 0)
        {
            outputBuffer.resize(bytes);
        }

        initialised = 0;

        return py::bytes(reinterpret_cast<const char *>(outputBuffer.data()), bytes);
    }

private:
    lame_global_flags *lame;
    int initialised;
};

PYBIND11_MODULE(pylame, m)
{
    py::class_<Encoder>(m, "Encoder")
        .def(py::init<>())
        .def("set_channels", &Encoder::set_channels)
        .def("set_quality", &Encoder::set_quality)
        .def("set_bitrate", &Encoder::set_bitrate)
        .def("set_in_sample_rate", &Encoder::set_in_sample_rate)
        .def("set_out_sample_rate", &Encoder::set_out_sample_rate)
        .def("encode", &Encoder::encode)
        .def("flush", &Encoder::flush)
        .def("silence", &Encoder::silence);
}
