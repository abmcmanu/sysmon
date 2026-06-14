class Sysmon < Formula
  desc "Small dependency-free system monitoring daemon"
  homepage "https://github.com/abmcmanu/sysmon"
  url "https://github.com/abmcmanu/sysmon/archive/refs/tags/v4.2.0.tar.gz"
  license "MIT"
  head "https://github.com/abmcmanu/sysmon.git", branch: "main"

  def install
    system "make", "install", "PREFIX=#{prefix}"
    (etc/"sysmond.conf").write <<~EOS unless (etc/"sysmond.conf").exist?
      [core]
      socket = #{var}/run/sysmond.sock
      [plugins]
      dir = #{lib}/sysmon/plugins
      load = uptime
      load = net
      load = cpustat
      [http]
      port = 9273
    EOS
  end

  service do
    run [opt_bin/"sysmond", "-f", "-c", etc/"sysmond.conf"]
    keep_alive true
    log_path var/"log/sysmond.log"
    error_log_path var/"log/sysmond.log"
  end

  test do
    assert_match "sysmond", shell_output("#{bin}/sysmond -V")
  end
end
