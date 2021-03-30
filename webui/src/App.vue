<template>
  <div id="app">
    <div class="container">
      <div class="container text-center">
        <h1>Electra32 Home Assistant configuration</h1>
        <div class="spinner-border" role="status" v-if="!ready">
          <span class="visually-hidden">Loading...</span>
        </div>
      </div>
      <form>
        <div class="row" v-if="ready">
          <div class="col col-6 mb-3">
            <label for="ssid" class="form-label">WiFi SSID</label>
            <input type="text" class="form-control" id="ssid" v-model="ssid" />
          </div>
          <div class="col col-6 mb-3">
            <label for="pass" class="form-label">WiFi Password</label>
            <div class="input-group">
              <input :type="attrWifiPassType" class="form-control" id="pass" v-model="pass" />
              <button class="btn btn-primary" type="button" v-on:click="toggleShowWifiPass">
                <img class="icon" src="@/assets/fa/eye.svg?data" v-if="attrWifiPassType == 'password'" />
                <img class="icon" src="@/assets/fa/eye-slash.svg?data" v-else />
              </button>
            </div>
          </div>
          <div class="col col-12 mb-3">
            <label for="ssid" class="form-label">MQTT URL</label>
            <input type="text" class="form-control" id="mqttUrl" v-model="mqttUrl" />
          </div>
          <div class="col col-6 mb-3">
            <label for="ssid" class="form-label">MQTT Username</label>
            <input type="text" class="form-control" id="mqttUser" v-model="mqttUser" />
          </div>
          <div class="col col-6 mb-3">
            <label for="ssid" class="form-label">MQTT Password</label>
            <input type="text" class="form-control" id="mqttPass" v-model="mqttPass" />
          </div>
          <div class="col col-12 mb-3">
            <label for="ssid" class="form-label">HA Entity name</label>
            <input type="text" class="form-control" id="entity" v-model="entity" />
          </div>
          <div class="col col12 text-center mt-3">
            <a href="javascript:void(0)" class="btn btn-primary" :class="{ disabled: !configChanged || saving }" v-on:click="saveConfig">
              <div class="spinner-border spinner-border-sm" role="status" v-if="saving">
                <span class="visually-hidden">Loading...</span>
              </div>
              Save configuration
            </a>
          </div>
        </div>
      </form>
    </div>
    <div class="toast-container position-absolute p-3 bottom-0 start-50 translate-middle-x" v-if="errors.length > 0 || savingSuccess">
      <div class="toast d-flex text-white bg-danger align-items-center" :class="{ show: errors.length > 0 }">
        <div class="toast-body">
          <ul>
            <li v-for="error in errors" :key="error">
              {{ error }}
            </li>
          </ul>
        </div>
        <button type="button" class="btn-close btn-close-white ms-auto me-2" v-on:click="clearErrors"></button>
      </div>
      <div class="toast d-flex text-white bg-success align-items-center" :class="{ show: savingSuccess }">
        <div class="toast-body">Configuration saved. ESP now rebooting.</div>
        <button type="button" class="btn-close btn-close-white ms-auto me-2" v-on:click="savingSuccess = false"></button>
      </div>
    </div>
  </div>
</template>

<script>
import axios from "axios";
import "./assets/bootstrap.min.css";

export default {
  name: "App",
  components: {},
  data: function () {
    return {
      config: {},
      ssid: "",
      pass: "",
      mqttUrl: "mqtts://",
      mqttUser: "",
      mqttPass: "",
      entity: "electra32",
      ready: false,
      errors: [],
      attrPasswordType: "password",
      attrWifiPassType: "password",
      attrAesType: "password",
      saving: false,
      savingSuccess: false,
    };
  },
  computed: {
    hasErrors() {
      return this.errors.length > 0;
    },
    configChanged() {
      const config = this.config;
      if (config.ssid != this.ssid) return true;
      if (config.pass != this.pass) return true;
      if (config.mqttUrl != this.mqttUrl) return true;
      if (config.mqttUser != this.mqttUser) return true;
      if (config.mqttPass != this.mqttPass) return true;
      if (config.entity != this.entity) return true;
      return false;
    },
  },
  methods: {
    async loadConfig() {
      try {
        const response = await axios.get("/config", {
          withCredentials: true,
        });
        const config = response.data;
        this.config = config;
        this.ssid = config.ssid;
        this.pass = config.pass;
        this.mqttUrl = config.mqttUrl || "mqtts://";
        this.mqttUser = config.mqttUser || "";
        this.mqttPass = config.mqttPass || "";
        this.entity = config.entity || "electra32";
        this.ready = true;
        return true;
      } catch (error) {
        if (typeof error.toString != "undefined") {
          this.errors.push(error.toString());
        } else {
          console.log(error);
          this.errors.push("Error fetching configuration data");
        }
      }
      return false;
    },
    async saveConfig() {
      if (this.configChanged && !this.saving) {
        this.saving = true;
        this.errors = [];
        // clone the current configuration so we can replace it later
        let config = JSON.parse(JSON.stringify(this.config));
        if (config.ssid != this.ssid) {
          config.ssid = this.ssid;
        }
        if (config.pass != this.pass) {
          config.pass = this.pass;
        }
        if (config.mqttUrl != this.mqttUrl) {
          config.mqttUrl = this.mqttUrl;
        }
        if (config.mqttUser != this.mqttUser) {
          config.mqttUser = this.mqttUser;
        }
        if (config.mqttPass != this.mqttPass) {
          config.mqttPass = this.mqttPass;
        }
        if (config.entity != this.entity) {
          config.entity = this.entity;
        }
        try {
          const response = await axios.post("/config", config, {
            withCredentials: true,
          });
          if (response.data == "OK") {
            this.config = config;
            this.savingSuccess = true;
            // setTimeout(() => {
            //   window.location.href = "http://electra32.local";
            // }, 2000);
          } else {
            this.errors.push("Error saving configuration data");
          }
        } catch (error) {
          if (typeof error.toString != "undefined") {
            this.errors.push(error.toString());
          } else {
            console.log(error);
            this.errors.push("Error saving configuration data");
          }
        }
        this.saving = false;
      }
    },
    toggleShowWifiPass() {
      if (this.attrWifiPassType == "password") {
        this.attrWifiPassType = "text";
      } else {
        this.attrWifiPassType = "password";
      }
    },
    clearErrors() {
      this.errors = [];
    },
  },
  async created() {
    await this.loadConfig();
  },
};
</script>
<style scoped>
.icon {
  width: 1.2em;
  display: inline-block;
  text-transform: none;
  line-height: 1;
  vertical-align: text-bottom;
  -webkit-font-smoothing: antialiased;
  -moz-osx-font-smoothing: grayscale;
  filter: invert(1);
}
</style>