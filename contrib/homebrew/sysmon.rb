class Sysmon < Formula
  desc "Small dependency-free system monitoring daemon"
  homepage "https://github.com/abmcmanu/sysmon"
  url "https://github.com/abmcmanu/sysmon/archive/refs/tags/v4.1.0.tar.gz"
  license "MIT"
  head "https://github.com/abmcmanu/sysmon.git", branch: "main"

  def install
    system "make", "install", "PREFIX=#{prefix}"
  end

  test do
    assert_match "sysmond", shell_output("#{bin}/sysmond -V")
  end
end
