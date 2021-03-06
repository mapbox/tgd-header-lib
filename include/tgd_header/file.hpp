#ifndef TGD_HEADER_FILE_HPP
#define TGD_HEADER_FILE_HPP

/*****************************************************************************

tgd_header - Encoding and decoding the Tiled Geographic Data Common Header.

This file is from https://github.com/mapbox/tgd-header-lib where you can find
more documentation.

*****************************************************************************/

/**
 * @file file.hpp
 *
 * @brief Contains the file class.
 */

#include <cassert>
#include <cstddef>
#include <fcntl.h>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <system_error>
#include <unistd.h>

#ifdef _WIN32
# ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN // Prevent winsock.h inclusion; avoid winsock2.h conflict
# endif
# include <crtdbg.h>
# include <io.h>
# include <windows.h>
#endif

#ifndef _MSC_VER
# include <unistd.h>
#endif

namespace tgd_header {

    namespace detail {

#ifdef _MSC_VER
        // Disable parameter validation on Windows and reenable it
        // automatically when scope closes.
        // https://docs.microsoft.com/en-us/cpp/c-runtime-library/parameter-validation
        class disable_invalid_parameter_handler {

            static void invalid_parameter_handler(
                    const wchar_t* expression,
                    const wchar_t* function,
                    const wchar_t* file,
                    unsigned int line,
                    uintptr_t pReserved
                    ) {
                // do nothing
            }

            _invalid_parameter_handler old_handler;
            int old_report_mode;

        public:

            disable_invalid_parameter_handler() :
                old_handler(_set_thread_local_invalid_parameter_handler(invalid_parameter_handler)),
                old_report_mode(_CrtSetReportMode(_CRT_ASSERT, 0)) {
            }

            ~disable_invalid_parameter_handler() {
                _CrtSetReportMode(_CRT_ASSERT, old_report_mode);
                _set_thread_local_invalid_parameter_handler(old_handler);
            }

        }; // class disable_invalid_parameter_handler
#endif

        class file {

            int m_fd = -1;

        protected:

            template <typename ...TArgs>
            static int open_file(const std::string& filename, int flags, TArgs... args) {
                const int fd = ::open(filename.c_str(), flags, args...); // NOLINT(cppcoreguidelines-pro-type-vararg,hicpp-vararg)

                if (fd < 0) {
                    throw std::system_error{errno, std::system_category(), std::string{"Error opening file '"} + filename + "': "};
                }

                return fd;
            }

        public:

            explicit file(int fd) noexcept :
                m_fd(fd) {
            }

            file(const file&) = delete;
            const file& operator=(const file&) = delete;

            file(file&& other) noexcept :
                m_fd(other.m_fd) {
                other.m_fd = -1;
            }

            file& operator=(file&& other) noexcept {
                assert(this != &other);
                try {
                    close();
                } catch (...) {
                    // ignore exceptions so that the move assignment
                    // can be noexcept
                }
                m_fd = other.m_fd;
                other.m_fd = -1;
                return *this;
            }

            ~file() noexcept {
                try {
                    close();
                } catch (...) {
                    // ignore errors so that the destructor can be noexcept
                }
            }

            void close() {
                if (m_fd >= 2) {
                    if (::close(m_fd) != 0) {
                        m_fd = -1;
                        throw std::system_error{errno, std::system_category(), "Error closing file: "};
                    }
                    m_fd = -1;
                }
            }

            int fd() const noexcept {
                return m_fd;
            }

            std::size_t file_size() const {
#ifdef _MSC_VER
                // Windows implementation
                disable_invalid_parameter_handler diph;
                // https://msdn.microsoft.com/en-us/library/dfbc2kec.aspx
                const auto size = ::_filelengthi64(m_fd);
                if (size < 0) {
                    throw std::system_error{errno, std::system_category(), "Could not get file size: "};
                }
                return static_cast<std::size_t>(size);
#else
                // Unix implementation
                struct stat s; // NOLINT(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
                if (::fstat(m_fd, &s) != 0) {
                    throw std::system_error{errno, std::system_category(), "Could not get file size: "};
                }
                return static_cast<std::size_t>(s.st_size);
#endif
            }

        }; // class file

    } // namespace detail

} // namespace tgd_header

#endif // TGD_HEADER_FILE_HPP
