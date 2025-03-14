#include <botan/auto_rng.h>
#include <botan/certstor.h>
#include <botan/tls.h>

/**
 * @brief Callbacks invoked by TLS::Channel.
 *
 * Botan::TLS::Callbacks is an abstract class.
 * For improved readability, only the functions that are mandatory
 * to implement are listed here. See src/lib/tls/tls_callbacks.h.
 */
class Callbacks : public Botan::TLS::Callbacks {
   public:
      void tls_emit_data([[maybe_unused]] std::span<const uint8_t> data) override {
         // send data to tls server, e.g., using BSD sockets or boost asio
      }

      void tls_record_received([[maybe_unused]] uint64_t seq_no,
                               [[maybe_unused]] std::span<const uint8_t> data) override {
         // process full TLS record received by tls server, e.g.,
         // by passing it to the application
      }

      void tls_alert([[maybe_unused]] Botan::TLS::Alert alert) override {
         // handle a tls alert received from the tls server
      }
};

/**
 * @brief Credentials storage for the tls client.
 *
 * It returns a list of trusted CA certificates from a local directory.
 * TLS client authentication is disabled. See src/lib/tls/credentials_manager.h.
 */
class Client_Credentials : public Botan::Credentials_Manager {
   public:
      std::vector<Botan::Certificate_Store*> trusted_certificate_authorities(
         [[maybe_unused]] const std::string& type, [[maybe_unused]] const std::string& context) override {
         // return a list of certificates of CAs we trust for tls server certificates,
         // e.g., all the certificates in the local directory "cas"
         return {&m_cert_store};
      }

   private:
      Botan::Certificate_Store_In_Memory m_cert_store{"cas"};
};

class Client_Policy : public Botan::TLS::Default_Policy {
   public:
      // This needs to be overridden to enable the hybrid PQ/T groups
      // additional to the default (classical) key exchange groups
      std::vector<Botan::TLS::Group_Params> key_exchange_groups() const override {
         auto groups = Botan::TLS::Default_Policy::key_exchange_groups();
         groups.push_back(Botan::TLS::Group_Params::HYBRID_X25519_ML_KEM_768);
         groups.push_back(Botan::TLS::Group_Params::HYBRID_SECP256R1_ML_KEM_768);
         groups.push_back(Botan::TLS::Group_Params::HYBRID_SECP384R1_ML_KEM_1024);
         return groups;
      }

      // Define that the client should exclusively pre-offer hybrid groups
      // in its initial Client Hello.
      std::vector<Botan::TLS::Group_Params> key_exchange_groups_to_offer() const override {
         return {Botan::TLS::Group_Params::HYBRID_X25519_ML_KEM_768};
      }
};

int main() {
   // prepare all the parameters
   auto rng = std::make_shared<Botan::AutoSeeded_RNG>();
   auto callbacks = std::make_shared<Callbacks>();
   auto session_mgr = std::make_shared<Botan::TLS::Session_Manager_In_Memory>(rng);
   auto creds = std::make_shared<Client_Credentials>();
   auto policy = std::make_shared<Botan::TLS::Strict_Policy>();

   // open the tls connection
   Botan::TLS::Client client(callbacks,
                             session_mgr,
                             creds,
                             policy,
                             rng,
                             Botan::TLS::Server_Information("botan.randombit.net", 443),
                             Botan::TLS::Protocol_Version::TLS_V12);

   while(!client.is_closed()) {
      // read data received from the tls server, e.g., using BSD sockets or boost asio
      // ...

      // send data to the tls server using client.send()
   }

   return 0;
}
