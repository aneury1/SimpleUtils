
#include <memory>
#include <cstdint>
#include <string>
#include <openssl/ssl.h>

namespace sslHelper{

const int RSA_KEY_LENGTH = 4096;
namespace {
 typedef uint8_t uchar;
}

bool generateX509(const std::string& certFileName, const std::string& keyFileName, long daysValid)
{
    bool result = false;

    std::unique_ptr<BIO, void (*)(BIO *)> certFile  { BIO_new_file(certFileName.data(), "wb"), BIO_free_all  };
    std::unique_ptr<BIO, void (*)(BIO *)> keyFile { BIO_new_file(keyFileName.data(), "wb"), BIO_free_all };

    if (certFile && keyFile)
    {
        std::unique_ptr<RSA, void (*)(RSA *)> rsa { RSA_new(), RSA_free };
        std::unique_ptr<BIGNUM, void (*)(BIGNUM *)> bn { BN_new(), BN_free };

        BN_set_word(bn.get(), RSA_F4);
        int rsa_ok = RSA_generate_key_ex(rsa.get(), RSA_KEY_LENGTH, bn.get(), nullptr);

        if (rsa_ok == 1)
        {
            // --- cert generation ---
            std::unique_ptr<X509, void (*)(X509 *)> cert { X509_new(), X509_free };
            std::unique_ptr<EVP_PKEY, void (*)(EVP_PKEY *)> pkey { EVP_PKEY_new(), EVP_PKEY_free};

            // The RSA structure will be automatically freed when the EVP_PKEY structure is freed.
            EVP_PKEY_assign(pkey.get(), EVP_PKEY_RSA, reinterpret_cast<char*>(rsa.release()));
            ASN1_INTEGER_set(X509_get_serialNumber(cert.get()), 1); // serial number

            X509_gmtime_adj(X509_get_notBefore(cert.get()), 0); // now
            X509_gmtime_adj(X509_get_notAfter(cert.get()), daysValid * 24 * 3600); // accepts secs

            X509_set_pubkey(cert.get(), pkey.get());

            // 1 -- X509_NAME may disambig with wincrypt.h
            // 2 -- DO NO FREE the name internal pointer
            X509_name_st* name = X509_get_subject_name(cert.get());

            const uchar country[] = "RU";
            const uchar company[] = "MyCompany, PLC";
            const uchar common_name[] = "localhost";

            X509_NAME_add_entry_by_txt(name, "C",  MBSTRING_ASC, country, -1, -1, 0);
            X509_NAME_add_entry_by_txt(name, "O",  MBSTRING_ASC, company, -1, -1, 0);
            X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, common_name, -1, -1, 0);

            X509_set_issuer_name(cert.get(), name);
            X509_sign(cert.get(), pkey.get(), EVP_sha256()); // some hash type here


            int ret  = PEM_write_bio_PrivateKey(keyFile.get(), pkey.get(), nullptr, nullptr, 0, nullptr, nullptr);
            int ret2 = PEM_write_bio_X509(certFile.get(), cert.get());

            result = (ret == 1) && (ret2 == 1); // OpenSSL return codes
        }
    }

    return result;
}


};
